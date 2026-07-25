#include "app/AppNotifier.h"

#include <QMessageBox>
#include <QPushButton>
#include <QSystemTrayIcon>

QtAppNotifier::QtAppNotifier(QSystemTrayIcon *tray, QWidget *dialogParent)
    : m_tray(tray)
    , m_dialogParent(dialogParent)
{
}

void QtAppNotifier::notifyPetLoadError(const QString &title, const QString &body)
{
    // showMessage is a local macOS notification (no network). Requires a visible
    // status item; guarded so a headless/offscreen run is a no-op.
    if (m_tray && QSystemTrayIcon::supportsMessages()) {
        m_tray->showMessage(title, body, QSystemTrayIcon::Warning, 5000);
    }
}

AppNotifier::PermissionChoice QtAppNotifier::promptInputPermission(const QString &title,
                                                                   const QString &body,
                                                                   const QString &openLabel)
{
    QMessageBox box(QMessageBox::Information, title, body, QMessageBox::Cancel, m_dialogParent);
    QPushButton *openButton = box.addButton(openLabel, QMessageBox::AcceptRole);
    box.exec();
    return box.clickedButton() == openButton ? PermissionChoice::OpenSettings
                                             : PermissionChoice::Dismiss;
}

void QtAppNotifier::warn(const QString &title, const QString &message)
{
    QMessageBox::warning(m_dialogParent, title, message);
}

bool QtAppNotifier::confirmRemoval(const QString &title, const QString &question)
{
    return QMessageBox::question(m_dialogParent, title, question) == QMessageBox::Yes;
}

void QtAppNotifier::showFatalStartup(const QString &title, const QString &message)
{
    QMessageBox::critical(m_dialogParent, title, message);
}
