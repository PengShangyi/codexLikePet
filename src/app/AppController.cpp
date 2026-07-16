#include "app/AppController.h"

#include "platform/MacApplication.h"
#include "pet/PetWindow.h"
#include "settings/AppSettings.h"
#include "settings/Localization.h"
#include "settings/SettingsWindow.h"
#include "pet/AnimationPlayer.h"
#include "pet/AtlasCache.h"
#include "pet/BehaviorController.h"
#include "quotes/QuoteProvider.h"
#include "quotes/SpeechBubble.h"
#include "resources/PetLibrary.h"
#include "resources/PetPackageImporter.h"
#include "resources/PetStore.h"
#include "input/MacInputActivitySource.h"
#include "input/TypingActivityDetector.h"

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

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_trayIcon(new QSystemTrayIcon(this))
    , m_menu(new QMenu)
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
    , m_quoteProvider(new FixedQuoteProvider(QStringLiteral("test balabala"), this))
    , m_speechBubble(new SpeechBubble)
    , m_inputSource(new MacInputActivitySource(this))
    , m_typingDetector(new TypingActivityDetector(m_inputSource, 1500, this))
{
    connect(m_settingsWindow, &SettingsWindow::resetPositionRequested, m_petWindow, &PetWindow::resetPosition);
    connect(m_localization, &Localization::languageChanged, this, &AppController::updateVisibilityAction);
    connect(m_settingsWindow, &SettingsWindow::petSelected, this, &AppController::selectPet);
    connect(m_settingsWindow, &SettingsWindow::importPackageRequested, this, [this] { importPet(false); });
    connect(m_settingsWindow, &SettingsWindow::importDirectoryRequested, this, [this] { importPet(true); });
    connect(m_settingsWindow, &SettingsWindow::removePetRequested, this, &AppController::removeSelectedPet);
    connect(m_animationPlayer, &AnimationPlayer::frameReady, m_petWindow, &PetWindow::setFrame);
    connect(m_settings, &AppSettings::animationSpeedChanged, m_animationPlayer, &AnimationPlayer::setSpeedFactor);
    connect(m_petWindow, &PetWindow::dragStarted, m_behavior, &BehaviorController::beginDrag);
    connect(m_petWindow, &PetWindow::dragDirectionChanged, m_behavior, &BehaviorController::setDragDirection);
    connect(m_petWindow, &PetWindow::dragFinished, m_behavior, &BehaviorController::endDrag);
    connect(m_petWindow, &PetWindow::clicked, this, &AppController::handlePetClick);
    connect(m_behavior, &BehaviorController::stateChanged, this, &AppController::applyBehaviorState);
    connect(m_animationPlayer, &AnimationPlayer::loopCompleted, this, [this](V2AnimationState state) {
        if (state == V2AnimationState::Waving && m_behavior->state() == BehaviorState::ClickReaction) {
            m_behavior->finishClickReaction();
        }
    });
    connect(m_quoteProvider, &QuoteProvider::quoteReady, this, [this](const QUuid &, const Quote &quote) {
        if (QScreen *screen = QGuiApplication::primaryScreen()) {
            m_speechBubble->showMessage(quote.text, m_petWindow->geometry(), screen->availableGeometry(), 3000);
        }
    });
    connect(m_typingDetector, &TypingActivityDetector::typingChanged, m_behavior, &BehaviorController::setTypingActive);
    connect(m_settings, &AppSettings::typingDetectionEnabledChanged, this, &AppController::setTypingMonitoringEnabled);
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
    m_petWindow->restorePosition();
    m_petWindow->show();
    refreshPetLibrary();
    setTypingMonitoringEnabled(m_settings->typingDetectionEnabled());

    connect(this, &AppController::petVisibilityRequested, m_petWindow, &QWidget::setVisible);
    return true;
}

void AppController::setTypingMonitoringEnabled(bool enabled)
{
    if (!enabled) {
        m_inputSource->stop();
        m_typingDetector->reset();
        return;
    }
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
    selectPet(selected);
}

void AppController::selectPet(const QString &id)
{
    const PetRecord *record = m_petLibrary->find(id);
    if (!record) {
        m_animationPlayer->stop();
        m_currentAtlas.clear();
        m_petWindow->setFrame({});
        m_settingsWindow->setPreviewAtlas({}, true);
        return;
    }
    QString error;
    const QString atlasPath = QDir(record->package.rootPath).filePath(record->package.spriteSheetPath);
    const QSharedPointer<PetAtlas> atlas = m_atlasCache->load(atlasPath, &error);
    if (!atlas) {
        m_settingsWindow->setValidationReport(error, true);
        return;
    }
    m_settings->setSelectedPetId(id);
    m_currentAtlas = atlas;
    m_animationPlayer->setAtlas(atlas);
    m_behavior->setSnapEdge(m_petWindow->snapEdge());
    applyBehaviorState(m_behavior->state());
    m_animationPlayer->setSpeedFactor(m_settings->animationSpeed());
    m_settingsWindow->setPreviewAtlas(atlas, record->package.renderMode == RenderMode::Smooth);
    m_settingsWindow->setValidationReport({}, false);
    m_settingsWindow->setPets(m_petLibrary->pets(), id);
}

void AppController::applyBehaviorState(BehaviorState state)
{
    if (!m_currentAtlas) return;
    switch (state) {
    case BehaviorState::Idle:
        m_animationPlayer->setState(V2AnimationState::Idle);
        m_animationPlayer->start();
        break;
    case BehaviorState::Typing:
        m_animationPlayer->setState(V2AnimationState::Running);
        m_animationPlayer->start();
        break;
    case BehaviorState::ClickReaction:
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
    case BehaviorState::EdgeLeft:
    case BehaviorState::EdgeRight:
    case BehaviorState::EdgeBottom: {
        m_animationPlayer->stop();
        const int lookIndex = state == BehaviorState::EdgeLeft ? 4
            : state == BehaviorState::EdgeRight ? 12 : 0;
        m_petWindow->setFrame(m_currentAtlas->lookFrame(lookIndex));
        break;
    }
    }
}

void AppController::handlePetClick()
{
    m_behavior->triggerClick();
    m_quoteProvider->requestQuote({m_settings->selectedPetId(), QStringLiteral("click"), {}},
                                  QUuid::createUuid());
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
}
