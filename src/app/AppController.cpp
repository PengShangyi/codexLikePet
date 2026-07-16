#include "app/AppController.h"

#include "platform/MacApplication.h"

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
{
}

AppController::~AppController()
{
    m_trayIcon->setContextMenu(nullptr);
    delete m_menu;
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

    QAction *settingsAction = m_menu->addAction(tr("Settings…"));
    connect(settingsAction, &QAction::triggered, this, &AppController::requestSettings);

    m_menu->addSeparator();
    QAction *quitAction = m_menu->addAction(tr("Quit Potato"));
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    m_trayIcon->setIcon(makeTrayIcon());
    m_trayIcon->setToolTip(QStringLiteral("Potato"));
    m_trayIcon->setContextMenu(m_menu);
    m_trayIcon->show();
    return true;
}

void AppController::requestSettings()
{
    MacApplication::activateIgnoringOtherApps();
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
        m_visibilityAction->setText(m_petVisible ? tr("Hide Pet") : tr("Show Pet"));
    }
}
