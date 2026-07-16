#pragma once

#include <QObject>
#include <QString>

class QLocalServer;

class SingleInstance final : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(QString serverName, QObject *parent = nullptr);

    bool acquire();
    bool isPrimary() const;
    bool notifyPrimary(const QString &message, int timeoutMs = 500) const;

signals:
    void messageReceived(const QString &message);

private:
    void acceptConnections();

    QString m_serverName;
    QLocalServer *m_server;
    bool m_primary = false;
};
