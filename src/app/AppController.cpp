#include "app/AppController.h"

#include "platform/MacApplication.h"
#include "pet/PetWindow.h"
#include "settings/AppSettings.h"
#include "settings/Localization.h"
#include "settings/SettingsWindow.h"
#include "pet/AnimationPlayer.h"
#include "pet/AnimationClip.h"
#include "pet/AtlasCache.h"
#include "pet/BehaviorController.h"
#include "quotes/QuoteProvider.h"
#include "quotes/SpeechBubble.h"
#include "resources/PetLibrary.h"
#include "resources/PetPackageImporter.h"
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
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>
#include <QDesktopServices>
#include <QPushButton>
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
    : QObject(parent)
    , m_trayIcon(new QSystemTrayIcon(this))
    , m_menu(new QMenu)
    , m_petMenu(nullptr)
    , m_visibilityAction(nullptr)
    , m_settingsAction(nullptr)
    , m_quitAction(nullptr)
    , m_settings(new AppSettings(this))
    , m_localization(new Localization(m_settings, this))
    , m_settingsWindow(new SettingsWindow(m_settings, m_localization))
    , m_petWindow(new PetWindow(m_settings))
    , m_petLibrary(new PetLibrary({}, {}, this))
    , m_importer(new PetPackageImporter(PetStore()))
    , m_atlasCache(new AtlasCache(2))
    , m_animationPlayer(new AnimationPlayer(this))
    , m_behavior(new BehaviorController(this))
    , m_idleScheduler(new IdleActivityScheduler({}, this))
    , m_quoteProvider(new LocalQuoteProvider(
          [localization = m_localization] { return localization->usesChinese(); },
          this))
    , m_speechBubble(new SpeechBubble)
    , m_inputSource(new MacInputActivitySource(this))
    , m_typingDetector(new TypingActivityDetector(m_inputSource, 1500, this))
    , m_environmentClock(new SystemEnvironmentClock(this))
    , m_environmentResolver(new EnvironmentResolver(m_settings, m_environmentClock, this))
    , m_systemActivity(new MacSystemActivitySource(this))
    , m_motionController(new MotionController(m_settings, m_systemActivity, this))
    , m_loginItemController(new MacLoginItemController)
    , m_loginItemCoordinator(new LoginItemCoordinator(m_settings, m_loginItemController, this))
    , m_clickCompletionTimer(new QTimer(this))
    , m_typingReturnTimer(new QTimer(this))
    , m_suppressSystemMutations(mode == AppRunMode::RuntimeCheck)
{
    m_clickCompletionTimer->setSingleShot(true);
    m_clickCompletionTimer->setInterval(500);
    connect(m_clickCompletionTimer, &QTimer::timeout, this, [this] {
        if (m_behavior->state() == BehaviorState::ClickReaction) {
            m_behavior->finishClickReaction();
        }
    });
    m_typingReturnTimer->setSingleShot(true);
    m_typingReturnTimer->setInterval(110);
    connect(m_typingReturnTimer, &QTimer::timeout, this, [this] {
        // Relax back to the resting "hands on keyboard" frame after a typing pause.
        if (m_typingPressActive && m_behavior->state() == BehaviorState::Typing) {
            m_animationPlayer->showClipFrame(0);
        }
    });
    connect(m_settingsWindow, &SettingsWindow::resetPositionRequested, m_petWindow, &PetWindow::resetPosition);
    connect(m_localization, &Localization::languageChanged, this, &AppController::updateVisibilityAction);
    connect(m_localization, &Localization::languageChanged, this, &AppController::configurePetPreview);
    connect(m_settingsWindow, &SettingsWindow::petSelected, this, &AppController::selectPet);
    connect(m_settingsWindow, &SettingsWindow::importPackageRequested, this, [this] { importPet(false); });
    connect(m_settingsWindow, &SettingsWindow::importDirectoryRequested, this, [this] { importPet(true); });
    connect(m_settingsWindow, &SettingsWindow::removePetRequested, this, &AppController::removeSelectedPet);
    connect(m_settingsWindow, &SettingsWindow::previewAtlasSelected, this, &AppController::loadPreviewAtlas);
    connect(m_settingsWindow, &SettingsWindow::previewClipSelected, this, &AppController::loadPreviewClip);
    connect(m_animationPlayer, &AnimationPlayer::frameReady, m_petWindow, &PetWindow::setFrame);
    connect(m_settings, &AppSettings::animationSpeedChanged, m_animationPlayer, &AnimationPlayer::setSpeedFactor);
    connect(m_petWindow, &PetWindow::dragStarted, m_behavior, &BehaviorController::beginDrag);
    connect(m_petWindow, &PetWindow::dragStarted, m_speechBubble, &QWidget::hide);
    connect(m_petWindow, &PetWindow::dragStarted, this, [this] { m_activeQuoteRequest = {}; });
    connect(m_petWindow, &PetWindow::dragDirectionChanged, m_behavior, &BehaviorController::setDragDirection);
    connect(m_petWindow, &PetWindow::dragFinished, m_behavior, &BehaviorController::endDrag);
    connect(m_petWindow, &PetWindow::snapEdgeChanged, m_behavior, &BehaviorController::setSnapEdge);
    connect(m_petWindow, &PetWindow::clicked, this, &AppController::handlePetClick);
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
            m_speechBubble, &SpeechBubble::setAlwaysOnTop);
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
    connect(m_motionController, &MotionController::reducedMotionChanged, m_settingsWindow, &SettingsWindow::setReducedMotion);
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
        QMessageBox::warning(m_settingsWindow,
                             m_localization->text(TextKey::LoginItemErrorTitle),
                             message);
    });
}

AppController::~AppController()
{
    m_trayIcon->setContextMenu(nullptr);
    delete m_menu;
    delete m_settingsWindow;
    delete m_petWindow;
    delete m_importer;
    delete m_atlasCache;
    delete m_speechBubble;
    delete m_loginItemController;
}

bool AppController::start()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return false;
    }

    QApplication::setQuitOnLastWindowClosed(false);
    MacApplication::setAccessoryActivationPolicy();

    m_visibilityAction = m_menu->addAction(QString());
    connect(m_visibilityAction, &QAction::triggered, this, [this] {
        setPetVisible(!m_petVisible);
    });
    updateVisibilityAction();

    m_petMenu = m_menu->addMenu(QString());

    m_settingsAction = m_menu->addAction(QString());
    connect(m_settingsAction, &QAction::triggered, this, &AppController::requestSettings);

    m_menu->addSeparator();
    m_quitAction = m_menu->addAction(QString());
    connect(m_quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    updateVisibilityAction();

    m_trayIcon->setIcon(makeTrayIcon());
    m_trayIcon->setToolTip(QStringLiteral("Potato"));
    m_trayIcon->setContextMenu(m_menu);
    m_trayIcon->show();
    // Right-clicking the pet pops the same menu at the cursor, so the tray icon
    // is a convenience rather than the only entry point.
    connect(m_petWindow, &PetWindow::contextMenuRequested, this, [this](const QPoint &globalPos) {
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

    connect(this, &AppController::petVisibilityRequested, m_petWindow, &QWidget::setVisible);
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
    QMessageBox box(QMessageBox::Information,
                    m_localization->text(TextKey::InputPermissionTitle),
                    m_localization->text(TextKey::InputPermissionBody),
                    QMessageBox::Cancel,
                    m_settingsWindow);
    QPushButton *openButton = box.addButton(m_localization->text(TextKey::OpenSystemSettings),
                                            QMessageBox::AcceptRole);
    box.exec();
    if (box.clickedButton() == openButton) {
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
        m_clipCache.clear();
        return;
    }
    m_settings->setSelectedPetId(id);
    updatePetMenu(id);
    m_currentPackage = record->package;
    configurePetPreview();
    loadCurrentVariant();
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
        return;
    }
    m_currentAtlas = atlas;
    m_clipCache.clear();
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

    QStringList summary;
    const QVector<Season> seasons{Season::Spring, Season::Summer, Season::Autumn, Season::Winter};
    const QVector<TimePhase> phases{TimePhase::Day, TimePhase::Night};
    const QStringList clipNames{QStringLiteral("click"),
                                QStringLiteral("typing"),
                                QStringLiteral("edge-left"),
                                QStringLiteral("edge-right"),
                                QStringLiteral("edge-bottom")};
    const QString builtInFallback = m_localization->usesChinese()
        ? QStringLiteral("v2 内置回退")
        : QStringLiteral("v2 fallback");
    for (const Season season : seasons) {
        for (const TimePhase phase : phases) {
            const VariantKey key{season, phase};
            QStringList clipSummary;
            for (const QString &clipName : clipNames) {
                const auto clip = EnvironmentResolver::clipDefinition(*m_currentPackage, key, clipName);
                clipSummary.append(QStringLiteral("%1=%2")
                                       .arg(clipName,
                                            clip ? clip->path : builtInFallback));
            }
            summary.append(QStringLiteral("%1 → %2\n  %3")
                               .arg(key.combinedName(),
                                    EnvironmentResolver::atlasRelativePath(*m_currentPackage, key),
                                    clipSummary.join(QStringLiteral("; "))));
        }
    }
    m_settingsWindow->setResourceSummary(summary.join(QLatin1Char('\n')));
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

    const QString path = QDir(m_currentPackage->rootPath).absoluteFilePath(definition->path);
    QStringList durationParts;
    durationParts.reserve(definition->durationsMs.size());
    for (const int duration : definition->durationsMs) {
        durationParts.append(QString::number(duration));
    }
    const QString cacheKey = path + QLatin1Char('|') + durationParts.join(QLatin1Char(','));
    const auto cached = m_clipCache.constFind(cacheKey);
    if (cached != m_clipCache.cend()) return *cached;

    auto clip = QSharedPointer<AnimationClip>::create();
    if (!clip->load(path, definition->durationsMs)) return {};
    m_clipCache.insert(cacheKey, clip);
    return clip;
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
    const QSharedPointer<AnimationClip> clip = loadClip(QStringLiteral("typing"));
    if (clip) {
        // Keystroke-driven: hold the clip on its resting frame and let each key
        // advance a press (onTypingKey/pulseTypingPress) instead of free-running.
        m_typingClipFrames = clip->frameCount();
        m_animationPlayer->setClip(clip, QStringLiteral("typing"), true);
        m_animationPlayer->showClipFrame(0);  // rest: hands on the keyboard (stops the timer)
        // Reduced motion holds that static rest frame with no per-key motion.
        m_typingPressActive = !m_motionController->reducedMotion() && m_typingClipFrames >= 2;
        m_typingReturnTimer->stop();
        return;
    }
    // No dedicated typing clip: fall back to the v2 "running" row, defined by the
    // contract as "active task work or processing, not literal foot-running"
    // (hatch-pet/references/animation-rows.md) — a clearly-distinct working loop.
    m_typingPressActive = false;
    m_animationPlayer->setState(V2AnimationState::Running);
    m_animationPlayer->start();
}

void AppController::pulseTypingPress()
{
    if (m_typingClipFrames < 2) return;
    // Alternate press frames when the clip provides them (1 = left paw, 2 = right);
    // a 2-frame clip just toggles rest/press.
    m_typingPressToggle = !m_typingPressToggle;
    const int press = m_typingClipFrames >= 3 ? (m_typingPressToggle ? 1 : 2) : 1;
    m_animationPlayer->showClipFrame(press);
    m_typingReturnTimer->start();  // relax back to rest after a short pause
}

void AppController::onTypingKey()
{
    // One key = one paw press. Gated so it animates only during Typing and touches
    // nothing but the in-memory frame index (no key content is ever involved).
    if (!m_typingPressActive || m_sleeping || !m_petVisible) return;
    if (m_behavior->state() != BehaviorState::Typing) return;
    pulseTypingPress();
}

void AppController::applyBehaviorState(BehaviorState state)
{
    if (state != BehaviorState::ClickReaction) m_clickCompletionTimer->stop();
    // Any higher-priority behavior interrupts and clears an in-flight fidget.
    if (state != BehaviorState::Idle) m_activeFidget.reset();
    // Leaving Typing stops the keystroke-driven press machinery.
    if (state != BehaviorState::Typing) {
        m_typingPressActive = false;
        m_typingReturnTimer->stop();
    }
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
    return m_typingPressActive;
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
        ? QFileDialog::getExistingDirectory(m_settingsWindow,
                                            m_localization->text(TextKey::ImportDirectory))
        : QFileDialog::getOpenFileName(m_settingsWindow,
                                       m_localization->text(TextKey::ImportPackage),
                                       {},
                                       QStringLiteral("Potato Pet (*.potatopet)"));
    if (path.isEmpty()) return;
    const PetImportResult result = m_importer->importPath(path);
    if (!result.success) {
        m_settingsWindow->setValidationReport(result.error, true);
        QMessageBox::warning(m_settingsWindow,
                             m_localization->text(TextKey::ImportFailed),
                             result.error);
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
    if (QMessageBox::question(m_settingsWindow,
                              QStringLiteral("Potato"),
                              m_localization->text(TextKey::RemoveConfirmation)) != QMessageBox::Yes) return;
    QString error;
    if (!m_petLibrary->removeCustomPet(id, &error)) {
        QMessageBox::warning(m_settingsWindow, m_localization->text(TextKey::ImportFailed), error);
        return;
    }
    const QString fallback = m_petLibrary->firstAvailableId();
    if (!fallback.isEmpty()) m_settings->setSelectedPetId(fallback);
    refreshPetLibrary();
}

void AppController::requestSettings()
{
    MacApplication::activateIgnoringOtherApps();
    m_settingsWindow->show();
    m_settingsWindow->raise();
    m_settingsWindow->activateWindow();
    emit settingsRequested();
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
