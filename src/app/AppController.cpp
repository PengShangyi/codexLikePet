#include "app/AppController.h"

#include "app/AppNotifier.h"
#include "platform/MacApplication.h"
#include "pet/PetWindow.h"
#include "settings/AppSettings.h"
#include "settings/Localization.h"
#include "settings/SettingsWindow.h"
#include "settings/OnboardingWindow.h"
#include "settings/AboutWindow.h"
#include "pet/AnimationPlayer.h"
#include "pet/AnimationClip.h"
#include "pet/AtlasCache.h"
#include "pet/BehaviorController.h"
#include "pet/ClipCache.h"
#include "pet/TypingAnimationDriver.h"
#include "quotes/QuoteProvider.h"
#include "quotes/SpeechBubble.h"
#include "resources/PetLibrary.h"
#include "resources/PetPackageImporter.h"
#include "resources/PetResourceSummary.h"
#include "resources/PetStore.h"
#include "input/MacInputActivitySource.h"
#include "input/TypingActivityDetector.h"
#include "environment/EnvironmentClock.h"
#include "environment/EnvironmentResolver.h"
#include "accessibility/MotionController.h"
#include "platform/MacSystemActivitySource.h"
#include "login/LoginItemCoordinator.h"
#include "login/MacLoginItemController.h"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QGuiApplication>
#include <QScreen>
#include <QDesktopServices>
#include <QTimer>

namespace {
QIcon makeTrayIcon()
{
    QPixmap pixmap(36, 36);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(63, 45, 33), 2.0));
    painter.setBrush(QColor(205, 154, 92));
    painter.drawEllipse(QRectF(5, 7, 26, 22));
    painter.setBrush(QColor(63, 45, 33));
    painter.drawEllipse(QRectF(12, 15, 3, 3));
    painter.drawEllipse(QRectF(22, 15, 3, 3));
    painter.drawArc(QRectF(15, 16, 8, 7), 200 * 16, 140 * 16);
    return QIcon(pixmap);
}
}

AppController::AppController(AppRunMode mode, QObject *parent)
    : AppController(Dependencies{}, mode, parent)
{
}

AppController::AppController(Dependencies deps, AppRunMode mode, QObject *parent)
    : QObject(parent)
    , m_trayIcon(new QSystemTrayIcon(this))
    , m_menu(std::make_unique<QMenu>())
    , m_petMenu(nullptr)
    , m_visibilityAction(nullptr)
    , m_settingsAction(nullptr)
    , m_quitAction(nullptr)
    , m_settings(deps.settings ? deps.settings : new AppSettings(this))
    , m_localization(new Localization(m_settings, this))
    , m_settingsWindow(std::make_unique<SettingsWindow>(m_settings, m_localization))
    , m_petWindow(std::make_unique<PetWindow>(m_settings))
    , m_petLibrary(new PetLibrary(deps.builtInPetRoot, deps.userPetRoot, this))
    , m_importer(std::make_unique<PetPackageImporter>(PetStore()))
    , m_atlasCache(std::make_unique<AtlasCache>(2))
    // One environment's clip set is click + typing + three edges; 8 leaves room
    // for a preview alongside them without growing without bound.
    , m_clipCache(std::make_unique<ClipCache>(8))
    , m_animationPlayer(new AnimationPlayer(this))
    , m_behavior(new BehaviorController(this))
    , m_idleScheduler(new IdleActivityScheduler(deps.idlePolicy, this))
    , m_quoteProvider(deps.quoteProvider ? deps.quoteProvider : new LocalQuoteProvider(
          [localization = m_localization] { return localization->usesChinese(); },
          this))
    , m_speechBubble(std::make_unique<SpeechBubble>())
    , m_inputSource(deps.input ? deps.input : new MacInputActivitySource(this))
    , m_typingDetector(new TypingActivityDetector(m_inputSource, 1500, this))
    , m_environmentClock(deps.clock ? deps.clock : new SystemEnvironmentClock(this))
    , m_environmentResolver(new EnvironmentResolver(m_settings, m_environmentClock, this))
    , m_systemActivity(deps.systemActivity ? deps.systemActivity : new MacSystemActivitySource(this))
    , m_motionController(new MotionController(m_settings, m_systemActivity, this))
    , m_ownedLoginItem(deps.loginItem ? nullptr : std::make_unique<MacLoginItemController>())
    , m_loginItemController(deps.loginItem ? deps.loginItem : m_ownedLoginItem.get())
    , m_loginItemCoordinator(new LoginItemCoordinator(m_settings, m_loginItemController, this))
    , m_typingDriver(new TypingAnimationDriver(m_animationPlayer, this))
    , m_clickCompletionTimer(new QTimer(this))
    , m_suppressSystemMutations(mode == AppRunMode::RuntimeCheck)
{
    m_systemTrayAvailable = deps.systemTrayAvailable
        ? deps.systemTrayAvailable
        : std::function<bool()>([] { return QSystemTrayIcon::isSystemTrayAvailable(); });
    if (!deps.notifier) {
        m_ownedNotifier = std::make_unique<QtAppNotifier>(m_trayIcon, m_settingsWindow.get());
    }
    m_notifier = deps.notifier ? deps.notifier : m_ownedNotifier.get();
    m_onboardingWindow = std::make_unique<OnboardingWindow>(m_localization);
    m_aboutWindow = std::make_unique<AboutWindow>(m_localization);
    const auto applyPetAccessibility = [this] {
        m_petWindow->setAccessibleName(m_localization->text(TextKey::PetAccessibleName));
        m_petWindow->setAccessibleDescription(m_localization->text(TextKey::PetAccessibleDescription));
    };
    applyPetAccessibility();
    connect(m_localization, &Localization::languageChanged, this, applyPetAccessibility);
    m_clickCompletionTimer->setSingleShot(true);
    m_clickCompletionTimer->setInterval(500);
    connect(m_clickCompletionTimer, &QTimer::timeout, this, [this] {
        if (m_behavior->state() == BehaviorState::ClickReaction) {
            m_behavior->finishClickReaction();
        }
    });
    connect(m_settingsWindow.get(), &SettingsWindow::resetPositionRequested, m_petWindow.get(), &PetWindow::resetPosition);
    connect(m_localization, &Localization::languageChanged, this, &AppController::updateVisibilityAction);
    connect(m_localization, &Localization::languageChanged, this, &AppController::configurePetPreview);
    // The summary embeds a localized "v2 fallback" label, so it follows language.
    connect(m_localization, &Localization::languageChanged, this, &AppController::updateResourceSummary);
    connect(m_settingsWindow.get(), &SettingsWindow::petSelected, this, &AppController::selectPet);
    connect(m_settingsWindow.get(), &SettingsWindow::importPackageRequested, this, [this] { importPet(false); });
    connect(m_settingsWindow.get(), &SettingsWindow::importDirectoryRequested, this, [this] { importPet(true); });
    connect(m_settingsWindow.get(), &SettingsWindow::removePetRequested, this, &AppController::removeSelectedPet);
    connect(m_settingsWindow.get(), &SettingsWindow::previewAtlasSelected, this, &AppController::loadPreviewAtlas);
    connect(m_settingsWindow.get(), &SettingsWindow::previewClipSelected, this, &AppController::loadPreviewClip);
    connect(m_settingsWindow.get(), &SettingsWindow::aboutRequested, this, &AppController::showAbout);
    connect(m_animationPlayer, &AnimationPlayer::frameReady, m_petWindow.get(), &PetWindow::setFrame);
    connect(m_settings, &AppSettings::animationSpeedChanged, m_animationPlayer, &AnimationPlayer::setSpeedFactor);
    connect(m_petWindow.get(), &PetWindow::dragStarted, m_behavior, &BehaviorController::beginDrag);
    connect(m_petWindow.get(), &PetWindow::dragStarted, m_speechBubble.get(), &QWidget::hide);
    connect(m_petWindow.get(), &PetWindow::dragStarted, this, [this] { m_activeQuoteRequest = {}; });
    connect(m_petWindow.get(), &PetWindow::dragDirectionChanged, m_behavior, &BehaviorController::setDragDirection);
    connect(m_petWindow.get(), &PetWindow::dragFinished, m_behavior, &BehaviorController::endDrag);
    connect(m_petWindow.get(), &PetWindow::snapEdgeChanged, m_behavior, &BehaviorController::setSnapEdge);
    connect(m_petWindow.get(), &PetWindow::clicked, this, &AppController::handlePetClick);
    connect(m_behavior, &BehaviorController::stateChanged, this, &AppController::applyBehaviorState);
    connect(m_idleScheduler, &IdleActivityScheduler::fidgetRequested, this, &AppController::playIdleFidget);
    connect(m_animationPlayer, &AnimationPlayer::loopCompleted, this, [this](V2AnimationState state) {
        if (state == V2AnimationState::Waving && m_behavior->state() == BehaviorState::ClickReaction) {
            m_behavior->finishClickReaction();
            return;
        }
        // An idle fidget played one loop: return to the looping Idle row and let
        // the scheduler pick a fresh interval for the next one.
        if (m_activeFidget && state == *m_activeFidget) {
            m_activeFidget.reset();
            if (m_behavior->state() == BehaviorState::Idle) {
                applyBehaviorState(BehaviorState::Idle);
                m_idleScheduler->notifyFidgetFinished();
            }
        }
    });
    connect(m_animationPlayer, &AnimationPlayer::clipLoopCompleted, this, [this](const QString &name) {
        if (name == QStringLiteral("click")
            && m_behavior->state() == BehaviorState::ClickReaction) {
            m_behavior->finishClickReaction();
        }
    });
    connect(m_quoteProvider, &QuoteProvider::quoteReady, this, [this](const QUuid &requestId, const Quote &quote) {
        if (requestId != m_activeQuoteRequest || !m_petVisible || m_sleeping) return;
        if (QScreen *screen = QGuiApplication::primaryScreen()) {
            m_speechBubble->showMessage(quote.text, m_petWindow->geometry(), screen->availableGeometry(), 3000);
        }
    });
    connect(m_settings, &AppSettings::alwaysOnTopChanged,
            m_speechBubble.get(), &SpeechBubble::setAlwaysOnTop);
    connect(m_typingDetector, &TypingActivityDetector::typingChanged, m_behavior, &BehaviorController::setTypingActive);
    // Per-keystroke press. Connected AFTER the detector (which is constructed first),
    // so on each key the detector's transition to the Typing state runs before this
    // pulse — the key that starts typing already reads as a press. Timing only.
    connect(m_inputSource, &InputActivitySource::activityDetected, this, &AppController::onTypingKey);
    connect(m_inputSource, &InputActivitySource::monitoringInvalidated, this, [this] {
        m_inputSource->stop();
        m_settings->setTypingDetectionEnabled(false);
    });
    connect(m_settings, &AppSettings::typingDetectionEnabledChanged, this, &AppController::setTypingMonitoringEnabled);
    connect(m_environmentResolver, &EnvironmentResolver::environmentChanged, this, [this](const VariantKey &) {
        loadCurrentVariant();
    });
    connect(m_motionController, &MotionController::reducedMotionChanged, m_animationPlayer, &AnimationPlayer::setReducedMotion);
    connect(m_motionController, &MotionController::reducedMotionChanged, m_settingsWindow.get(), &SettingsWindow::setReducedMotion);
    connect(m_motionController, &MotionController::reducedMotionChanged, this, [this](bool reduced) {
        // A fidget frozen mid-loop by reduced motion would never emit
        // loopCompleted, so drop it and restore the (now static) Idle frame.
        if (reduced && m_activeFidget) {
            m_activeFidget.reset();
            if (m_behavior->state() == BehaviorState::Idle) applyBehaviorState(BehaviorState::Idle);
        }
        updateIdleScheduler();
        // Typing switches between per-keystroke motion and a static rest frame.
        if (m_behavior->state() == BehaviorState::Typing) {
            applyBehaviorState(BehaviorState::Typing);
            return;
        }
        if (m_behavior->state() != BehaviorState::ClickReaction) return;
        if (reduced) m_clickCompletionTimer->start();
        else m_clickCompletionTimer->stop();
    });
    connect(m_systemActivity, &SystemActivitySource::willSleep, this, &AppController::handleSystemSleep);
    connect(m_systemActivity, &SystemActivitySource::didWake, this, &AppController::handleSystemWake);
    connect(m_loginItemCoordinator, &LoginItemCoordinator::updateFailed, this, [this](const QString &message) {
        m_notifier->warn(m_localization->text(TextKey::LoginItemErrorTitle), message);
    });
}

AppController::~AppController()
{
    // The only teardown step ownership cannot express: the tray icon does not own
    // its context menu, so it must stop pointing at one we are about to destroy.
    m_trayIcon->setContextMenu(nullptr);
}

bool AppController::start()
{
    if (!m_systemTrayAvailable()) {
        return false;
    }

    QApplication::setQuitOnLastWindowClosed(false);
    if (!m_suppressSystemMutations) MacApplication::setAccessoryActivationPolicy();

    m_visibilityAction = m_menu->addAction(QString());
    connect(m_visibilityAction, &QAction::triggered, this, [this] {
        setPetVisible(!m_petVisible);
    });
    updateVisibilityAction();

    m_petMenu = m_menu->addMenu(QString());

    m_settingsAction = m_menu->addAction(QString());
    connect(m_settingsAction, &QAction::triggered, this, &AppController::requestSettings);

    m_welcomeAction = m_menu->addAction(QString());
    connect(m_welcomeAction, &QAction::triggered, this, &AppController::showWelcome);

    m_aboutAction = m_menu->addAction(QString());
    connect(m_aboutAction, &QAction::triggered, this, &AppController::showAbout);

    m_menu->addSeparator();
    m_quitAction = m_menu->addAction(QString());
    connect(m_quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    updateVisibilityAction();

    m_trayIcon->setIcon(makeTrayIcon());
    m_trayIcon->setToolTip(QStringLiteral("Potato"));
    m_trayIcon->setContextMenu(m_menu.get());
    m_trayIcon->show();
    // Right-clicking the pet pops the same menu at the cursor, so the tray icon
    // is a convenience rather than the only entry point.
    connect(m_petWindow.get(), &PetWindow::contextMenuRequested, this, [this](const QPoint &globalPos) {
        m_menu->popup(globalPos);
    });
    m_petWindow->restorePosition();
    m_petWindow->show();
    refreshPetLibrary();
    updateIdleScheduler();
    setTypingMonitoringEnabled(m_settings->typingDetectionEnabled());
    m_environmentResolver->start();
    m_animationPlayer->setReducedMotion(m_motionController->reducedMotion());
    m_settingsWindow->setReducedMotion(m_motionController->reducedMotion());
    m_speechBubble->setAlwaysOnTop(m_settings->alwaysOnTop());
    if (!m_suppressSystemMutations) m_loginItemCoordinator->initialize();

    if (!m_suppressSystemMutations && !m_settings->onboardingCompleted()) {
        showWelcome();
        m_settings->setOnboardingCompleted(true);
    }

    connect(this, &AppController::petVisibilityRequested, m_petWindow.get(), &QWidget::setVisible);
    return true;
}

void AppController::handleSystemSleep()
{
    m_sleeping = true;
    m_animationPlayer->stop();
    m_environmentResolver->stop();
    m_inputSource->stop();
    m_typingDetector->reset();
    m_speechBubble->hide();
    m_clickCompletionTimer->stop();
    m_activeFidget.reset();
    m_activeQuoteRequest = {};
    updateIdleScheduler();
}

void AppController::handleSystemWake()
{
    m_sleeping = false;
    // clampToPrimaryScreen() also re-pushes the overlay level, in case display
    // reconfiguration around sleep/wake dropped it.
    m_petWindow->clampToPrimaryScreen();
    m_environmentResolver->reevaluate();
    if (m_petVisible) {
        m_environmentResolver->start();
        setTypingMonitoringEnabled(m_settings->typingDetectionEnabled());
        applyBehaviorState(m_behavior->state());
        if (m_motionController->reducedMotion()
            && m_behavior->state() == BehaviorState::ClickReaction) {
            m_clickCompletionTimer->start();
        }
    }
    updateIdleScheduler();
}

void AppController::setTypingMonitoringEnabled(bool enabled)
{
    if (!enabled) {
        m_inputSource->stop();
        m_typingDetector->reset();
        return;
    }
    if (!m_petVisible || m_sleeping) return;
    const InputStartResult result = m_inputSource->start();
    if (result == InputStartResult::Started) return;

    m_settings->setTypingDetectionEnabled(false);
    m_typingDetector->reset();
    // First-time request: the system access prompt is already on screen. Don't
    // stack our own dialog on top of it; the user allows there, then re-enables.
    if (result == InputStartResult::PermissionRequested) return;
    const AppNotifier::PermissionChoice choice = m_notifier->promptInputPermission(
        m_localization->text(TextKey::InputPermissionTitle),
        m_localization->text(TextKey::InputPermissionBody),
        m_localization->text(TextKey::OpenSystemSettings));
    if (choice == AppNotifier::PermissionChoice::OpenSettings) {
        QDesktopServices::openUrl(QUrl(QStringLiteral("x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent")));
    }
}

void AppController::refreshPetLibrary()
{
    m_petLibrary->refresh();
    QString selected = m_settings->selectedPetId();
    if (!m_petLibrary->find(selected)) {
        selected = m_petLibrary->firstAvailableId();
        if (!selected.isEmpty()) m_settings->setSelectedPetId(selected);
    }
    m_settingsWindow->setPets(m_petLibrary->pets(), selected);
    updatePetMenu(selected);
    selectPet(selected);
}

void AppController::selectPet(const QString &id)
{
    m_speechBubble->hide();
    m_activeQuoteRequest = {};
    const PetRecord *record = m_petLibrary->find(id);
    if (!record) {
        m_animationPlayer->stop();
        m_currentAtlas.clear();
        m_petWindow->setFrame({});
        m_settingsWindow->setPreviewAtlas({}, true);
        m_settingsWindow->setPreviewOptions({}, {}, {});
        m_settingsWindow->setPreviewClipOptions({}, {});
        m_settingsWindow->setResourceSummary({});
        m_currentPackage.reset();
        m_clipCache->clear();
        return;
    }
    m_settings->setSelectedPetId(id);
    updatePetMenu(id);
    m_currentPackage = record->package;
    loadCurrentVariant();
    // After loadCurrentVariant, not before: it configures the preview itself, and
    // calling it here too rebuilt both combo lists twice per pet selection.
    configurePetPreview();
    updateResourceSummary();
    m_settingsWindow->setPets(m_petLibrary->pets(), id);
}

void AppController::loadCurrentVariant()
{
    if (!m_currentPackage) return;
    QString error;
    const QString relativePath = EnvironmentResolver::atlasRelativePath(*m_currentPackage,
                                                                         m_environmentResolver->current());
    const QString atlasPath = QDir(m_currentPackage->rootPath).filePath(relativePath);
    const QSharedPointer<PetAtlas> atlas = m_atlasCache->load(atlasPath, &error);
    if (!atlas) {
        m_settingsWindow->setValidationReport(error, true);
        m_notifier->notifyPetLoadError(m_localization->text(TextKey::PetLoadErrorTitle), error);
        return;
    }
    m_currentAtlas = atlas;
    m_clipCache->clear();
    m_animationPlayer->setAtlas(atlas);
    m_petWindow->setSmoothRendering(m_currentPackage->renderMode == RenderMode::Smooth);
    m_behavior->setSnapEdge(m_petWindow->snapEdge());
    applyBehaviorState(m_behavior->state());
    m_animationPlayer->setSpeedFactor(m_settings->animationSpeed());
    m_settingsWindow->setPreviewAtlas(atlas, m_currentPackage->renderMode == RenderMode::Smooth);
    m_settingsWindow->setValidationReport({}, false);
    configurePetPreview();
}

void AppController::configurePetPreview()
{
    if (!m_currentPackage) return;
    QStringList labels{QStringLiteral("default — %1").arg(m_currentPackage->spriteSheetPath)};
    QStringList paths{m_currentPackage->spriteSheetPath};
    QStringList variantNames = m_currentPackage->variants.keys();
    variantNames.sort(Qt::CaseInsensitive);
    for (const QString &name : variantNames) {
        const QString path = m_currentPackage->variants.value(name);
        labels.append(QStringLiteral("%1 — %2").arg(name, path));
        paths.append(path);
    }
    const QString selected = EnvironmentResolver::atlasRelativePath(*m_currentPackage,
                                                                     m_environmentResolver->current());
    m_settingsWindow->setPreviewOptions(labels, paths, selected);

    QStringList clipLabels;
    QStringList clipKeys;
    QStringList baseClipNames = m_currentPackage->clips.keys();
    baseClipNames.sort(Qt::CaseInsensitive);
    for (const QString &name : baseClipNames) {
        const ClipDefinition clip = m_currentPackage->clips.value(name);
        clipLabels.append(QStringLiteral("base / %1 — %2").arg(name, clip.path));
        clipKeys.append(QStringLiteral("base|%1").arg(name));
    }
    QStringList clipScopes = m_currentPackage->variantClips.keys();
    clipScopes.sort(Qt::CaseInsensitive);
    for (const QString &scope : clipScopes) {
        QStringList names = m_currentPackage->variantClips.value(scope).keys();
        names.sort(Qt::CaseInsensitive);
        for (const QString &name : names) {
            const ClipDefinition clip = m_currentPackage->variantClips.value(scope).value(name);
            clipLabels.append(QStringLiteral("%1 / %2 — %3").arg(scope, name, clip.path));
            clipKeys.append(QStringLiteral("%1|%2").arg(scope, name));
        }
    }
    m_settingsWindow->setPreviewClipOptions(clipLabels, clipKeys);
}

// The season/day-night fallback table. Depends only on the package and the UI
// language, so it is rebuilt on pet or language change -- not from
// loadCurrentVariant(), which also runs on every day/night and season rollover
// and used to redo all 40 lookups and their string building for nothing.
void AppController::updateResourceSummary()
{
    if (!m_currentPackage) return;
    const QString builtInFallback = m_localization->usesChinese()
        ? QStringLiteral("v2 内置回退")
        : QStringLiteral("v2 fallback");
    m_settingsWindow->setResourceSummary(
        PetResourceSummary::build(*m_currentPackage, builtInFallback));
}

void AppController::loadPreviewAtlas(const QString &relativePath)
{
    if (!m_currentPackage || relativePath.isEmpty()) return;
    QString error;
    const QString path = QDir(m_currentPackage->rootPath).filePath(relativePath);
    const QSharedPointer<PetAtlas> atlas = m_atlasCache->load(path, &error);
    if (!atlas) {
        m_settingsWindow->setValidationReport(error, true);
        return;
    }
    m_settingsWindow->setPreviewAtlas(atlas,
                                      m_currentPackage->renderMode == RenderMode::Smooth);
}

void AppController::loadPreviewClip(const QString &key)
{
    if (!m_currentPackage) return;
    const int separator = key.indexOf(QLatin1Char('|'));
    if (separator <= 0) return;
    const QString scope = key.left(separator);
    const QString name = key.mid(separator + 1);
    const auto clips = scope == QStringLiteral("base")
        ? m_currentPackage->clips
        : m_currentPackage->variantClips.value(scope);
    const auto definition = clips.constFind(name);
    if (definition == clips.cend()) return;

    auto clip = QSharedPointer<AnimationClip>::create();
    const QString path = QDir(m_currentPackage->rootPath).filePath(definition->path);
    if (!clip->load(path, definition->durationsMs)) {
        m_settingsWindow->setValidationReport(clip->errorString(), true);
        return;
    }
    m_settingsWindow->setPreviewClip(clip,
                                     m_currentPackage->renderMode == RenderMode::Smooth);
}

QSharedPointer<AnimationClip> AppController::loadClip(const QString &name)
{
    if (!m_currentPackage) return {};
    const std::optional<ClipDefinition> definition = EnvironmentResolver::clipDefinition(
        *m_currentPackage,
        m_environmentResolver->current(),
        name);
    if (!definition) return {};

    return m_clipCache->load(QDir(m_currentPackage->rootPath).absoluteFilePath(definition->path),
                             definition->durationsMs);
}

bool AppController::playClip(const QString &name, bool restart)
{
    const QSharedPointer<AnimationClip> clip = loadClip(name);
    if (!clip) return false;
    m_animationPlayer->setClip(clip, name, restart);
    m_animationPlayer->start();
    return true;
}

void AppController::beginTypingAnimation()
{
    m_typingDriver->begin(loadClip(QStringLiteral("typing")),
                          m_motionController->reducedMotion());
}

void AppController::onTypingKey()
{
    // One key = one paw press. Gated so it animates only while the pet is actually
    // typing on screen; the driver moves nothing but an in-memory frame index (no
    // key content is ever involved).
    if (m_sleeping || !m_petVisible) return;
    if (m_behavior->state() != BehaviorState::Typing) return;
    m_typingDriver->onKey();
}

void AppController::applyBehaviorState(BehaviorState state)
{
    if (state != BehaviorState::ClickReaction) m_clickCompletionTimer->stop();
    // Any higher-priority behavior interrupts and clears an in-flight fidget.
    if (state != BehaviorState::Idle) m_activeFidget.reset();
    // Leaving Typing stops the keystroke-driven press machinery.
    if (state != BehaviorState::Typing) m_typingDriver->stop();
    if (!m_currentAtlas || !m_petVisible || m_sleeping) {
        m_animationPlayer->stop();
        updateIdleScheduler();
        return;
    }
    switch (state) {
    case BehaviorState::Idle:
        m_animationPlayer->setState(V2AnimationState::Idle);
        m_animationPlayer->start();
        break;
    case BehaviorState::Typing:
        beginTypingAnimation();
        break;
    case BehaviorState::ClickReaction:
        if (playClip(QStringLiteral("click"))) break;
        m_animationPlayer->setState(V2AnimationState::Waving);
        m_animationPlayer->start();
        break;
    case BehaviorState::DraggingLeft:
        m_animationPlayer->setState(V2AnimationState::RunningLeft, false);
        m_animationPlayer->start();
        break;
    case BehaviorState::DraggingRight:
        m_animationPlayer->setState(V2AnimationState::RunningRight, false);
        m_animationPlayer->start();
        break;
    // TEMP: left/right edge interaction animations disabled per request.
    // The pet still snaps to the side edges but renders the plain idle loop
    // instead of the edge-left/edge-right clips. Restore the block below to
    // re-enable them.
    case BehaviorState::EdgeLeft:
    case BehaviorState::EdgeRight:
        m_animationPlayer->setState(V2AnimationState::Idle);
        m_animationPlayer->start();
        break;
    case BehaviorState::EdgeBottom: {
        if (playClip(QStringLiteral("edge-bottom"), false)) break;
        m_animationPlayer->stop();
        m_petWindow->setFrame(m_currentAtlas->lookFrame(0));
        break;
    }
    /* Original combined edge handling — restore to re-enable left/right edge
       interaction animations:
    case BehaviorState::EdgeLeft:
    case BehaviorState::EdgeRight:
    case BehaviorState::EdgeBottom: {
        const QString clipName = state == BehaviorState::EdgeLeft
            ? QStringLiteral("edge-left")
            : state == BehaviorState::EdgeRight ? QStringLiteral("edge-right")
                                                : QStringLiteral("edge-bottom");
        if (playClip(clipName, false)) break;
        m_animationPlayer->stop();
        const int lookIndex = state == BehaviorState::EdgeLeft ? 4
            : state == BehaviorState::EdgeRight ? 12 : 0;
        m_petWindow->setFrame(m_currentAtlas->lookFrame(lookIndex));
        break;
    }
    */
    }
    updateIdleScheduler();
}

void AppController::updateIdleScheduler()
{
    const bool active = m_behavior->state() == BehaviorState::Idle
        && m_petVisible && !m_sleeping
        && !m_motionController->reducedMotion()
        && !m_currentAtlas.isNull();
    m_idleScheduler->setActive(active);
}

void AppController::playIdleFidget(V2AnimationState state)
{
    // Re-check the full gate: the timer may fire in the window between arming and
    // a higher-priority transition, or after motion/visibility changed.
    if (m_behavior->state() != BehaviorState::Idle || !m_petVisible || m_sleeping
        || m_motionController->reducedMotion() || m_currentAtlas.isNull()) {
        return;
    }
    m_activeFidget = state;
    m_animationPlayer->setState(state, true);
    m_animationPlayer->start();
}

bool AppController::isIdleFidgetArmed() const
{
    return m_idleScheduler->isArmed();
}

bool AppController::isTypingPressActive() const
{
    return m_typingDriver->isPressActive();
}

void AppController::handlePetClick()
{
    const bool alreadyReacting = m_behavior->state() == BehaviorState::ClickReaction;
    m_behavior->triggerClick();
    if (alreadyReacting) applyBehaviorState(BehaviorState::ClickReaction);
    if (m_motionController->reducedMotion()) {
        m_clickCompletionTimer->start();
    } else m_clickCompletionTimer->stop();
    m_activeQuoteRequest = QUuid::createUuid();
    m_quoteProvider->requestQuote({m_settings->selectedPetId(),
                                   QStringLiteral("click"),
                                   m_environmentResolver->current().combinedName()},
                                  m_activeQuoteRequest);
}

void AppController::importPet(bool directory)
{
    const QString path = directory
        ? QFileDialog::getExistingDirectory(m_settingsWindow.get(),
                                            m_localization->text(TextKey::ImportDirectory))
        : QFileDialog::getOpenFileName(m_settingsWindow.get(),
                                       m_localization->text(TextKey::ImportPackage),
                                       {},
                                       QStringLiteral("Potato Pet (*.potatopet)"));
    if (path.isEmpty()) return;
    const PetImportResult result = m_importer->importPath(path);
    if (!result.success) {
        m_settingsWindow->setValidationReport(result.error, true);
        m_notifier->warn(m_localization->text(TextKey::ImportFailed), result.error);
        return;
    }
    m_settings->setSelectedPetId(result.validation.package.id);
    refreshPetLibrary();
    m_settingsWindow->setValidationReport(m_localization->text(TextKey::ImportSucceeded), false);
}

void AppController::removeSelectedPet()
{
    const QString id = m_settingsWindow->selectedPetId();
    const PetRecord *record = m_petLibrary->find(id);
    if (!record || record->builtIn) return;
    if (!m_notifier->confirmRemoval(QStringLiteral("Potato"),
                                    m_localization->text(TextKey::RemoveConfirmation))) return;
    QString error;
    if (!m_petLibrary->removeCustomPet(id, &error)) {
        m_notifier->warn(m_localization->text(TextKey::ImportFailed), error);
        return;
    }
    const QString fallback = m_petLibrary->firstAvailableId();
    if (!fallback.isEmpty()) m_settings->setSelectedPetId(fallback);
    refreshPetLibrary();
}

void AppController::requestSettings()
{
    if (!m_suppressSystemMutations) MacApplication::activateIgnoringOtherApps();
    m_settingsWindow->show();
    m_settingsWindow->raise();
    m_settingsWindow->activateWindow();
    emit settingsRequested();
}

void AppController::showWelcome()
{
    if (!m_suppressSystemMutations) MacApplication::activateIgnoringOtherApps();
    m_onboardingWindow->show();
    m_onboardingWindow->raise();
    m_onboardingWindow->activateWindow();
}

void AppController::showAbout()
{
    if (!m_suppressSystemMutations) MacApplication::activateIgnoringOtherApps();
    m_aboutWindow->show();
    m_aboutWindow->raise();
    m_aboutWindow->activateWindow();
}

void AppController::presentStartupFailure()
{
    if (m_suppressSystemMutations) return;
    m_notifier->showFatalStartup(m_localization->text(TextKey::StartupFailedTitle),
                                 m_localization->text(TextKey::StartupFailedBody));
}

void AppController::setPetVisible(bool visible)
{
    if (m_petVisible == visible) {
        return;
    }
    m_petVisible = visible;
    if (!visible) {
        m_animationPlayer->stop();
        m_environmentResolver->stop();
        m_inputSource->stop();
        m_typingDetector->reset();
        m_speechBubble->hide();
        m_clickCompletionTimer->stop();
        m_activeFidget.reset();
        m_activeQuoteRequest = {};
    } else if (!m_sleeping) {
        m_environmentResolver->start();
        setTypingMonitoringEnabled(m_settings->typingDetectionEnabled());
        applyBehaviorState(m_behavior->state());
        if (m_motionController->reducedMotion()
            && m_behavior->state() == BehaviorState::ClickReaction) {
            m_clickCompletionTimer->start();
        }
    }
    updateIdleScheduler();
    updateVisibilityAction();
    emit petVisibilityRequested(visible);
}

void AppController::updateVisibilityAction()
{
    if (m_visibilityAction) {
        m_visibilityAction->setText(m_petVisible ? m_localization->text(TextKey::HidePet)
                                                  : m_localization->text(TextKey::ShowPet));
    }
    if (m_settingsAction) m_settingsAction->setText(m_localization->text(TextKey::Settings));
    if (m_welcomeAction) m_welcomeAction->setText(m_localization->text(TextKey::WelcomeMenuItem));
    if (m_aboutAction) m_aboutAction->setText(m_localization->text(TextKey::AboutMenuItem));
    if (m_quitAction) m_quitAction->setText(m_localization->text(TextKey::Quit));
    if (m_petMenu) m_petMenu->setTitle(m_localization->text(TextKey::Pet));
}

void AppController::updatePetMenu(const QString &selectedId)
{
    if (!m_petMenu) return;
    const QVector<PetRecord> pets = m_petLibrary->pets();
    const QList<QAction *> existing = m_petMenu->actions();
    bool samePets = existing.size() == pets.size();
    if (samePets) {
        for (int index = 0; index < pets.size(); ++index) {
            if (existing.at(index)->data().toString() != pets.at(index).package.id) {
                samePets = false;
                break;
            }
        }
    }
    if (samePets) {
        for (QAction *action : existing) {
            action->setChecked(action->data().toString() == selectedId);
        }
        return;
    }

    m_petMenu->clear();
    if (pets.isEmpty()) {
        QAction *empty = m_petMenu->addAction(m_localization->text(TextKey::NoPetSelected));
        empty->setEnabled(false);
        return;
    }
    for (const PetRecord &record : pets) {
        QAction *action = m_petMenu->addAction(record.package.displayName);
        action->setData(record.package.id);
        action->setCheckable(true);
        action->setChecked(record.package.id == selectedId);
        const QString id = record.package.id;
        connect(action, &QAction::triggered, this, [this, id] {
            QTimer::singleShot(0, this, [this, id] { selectPet(id); });
        });
    }
}
