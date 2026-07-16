#include "app/AppController.h"

#include "platform/MacApplication.h"
#include "pet/PetWindow.h"
#include "settings/AppSettings.h"
#include "settings/Localization.h"
#include "settings/SettingsWindow.h"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>

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
{
    connect(m_settingsWindow, &SettingsWindow::resetPositionRequested, m_petWindow, &PetWindow::resetPosition);
    connect(m_localization, &Localization::languageChanged, this, &AppController::updateVisibilityAction);
}

AppController::~AppController()
{
    m_trayIcon->setContextMenu(nullptr);
    delete m_menu;
    delete m_settingsWindow;
    delete m_petWindow;
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

    connect(this, &AppController::petVisibilityRequested, m_petWindow, &QWidget::setVisible);
    return true;
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
