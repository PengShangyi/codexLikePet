#pragma once

#include <QString>

class QSystemTrayIcon;
class QWidget;

// Single boundary for every user-facing alert. Routing all QMessageBox dialogs
// and status-item notifications through one interface (a) centralizes error
// surfacing so failures never happen silently, and (b) lets tests drive
// AppController without real modal dialogs blocking the run.
class AppNotifier
{
public:
    enum class PermissionChoice {
        OpenSettings,
        Dismiss,
    };

    virtual ~AppNotifier() = default;

    // Non-blocking, local-only notification (no network). Used when the selected
    // pet fails to load so the empty pet window is explained rather than silent.
    virtual void notifyPetLoadError(const QString &title, const QString &body) = 0;

    // Modal prompt shown when input-monitoring permission is unavailable.
    virtual PermissionChoice promptInputPermission(const QString &title,
                                                   const QString &body,
                                                   const QString &openLabel) = 0;

    virtual void warn(const QString &title, const QString &message) = 0;

    // Returns true when the user confirms the destructive action.
    virtual bool confirmRemoval(const QString &title, const QString &question) = 0;

    // Shown when the app cannot start (e.g. no system tray) before it exits.
    virtual void showFatalStartup(const QString &title, const QString &message) = 0;
};

// Production implementation backed by Qt widgets + the status-item balloon.
class QtAppNotifier final : public AppNotifier
{
public:
    QtAppNotifier(QSystemTrayIcon *tray, QWidget *dialogParent);

    void notifyPetLoadError(const QString &title, const QString &body) override;
    PermissionChoice promptInputPermission(const QString &title,
                                           const QString &body,
                                           const QString &openLabel) override;
    void warn(const QString &title, const QString &message) override;
    bool confirmRemoval(const QString &title, const QString &question) override;
    void showFatalStartup(const QString &title, const QString &message) override;

private:
    QSystemTrayIcon *m_tray;
    QWidget *m_dialogParent;
};
