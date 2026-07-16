#include "app/SingleInstance.h"

#include <QLocalServer>
#include <QLocalSocket>

SingleInstance::SingleInstance(QString serverName, QObject *parent)
    : QObject(parent)
    , m_serverName(std::move(serverName))
    , m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstance::acceptConnections);
}

bool SingleInstance::acquire()
{
    QLocalSocket probe;
    probe.connectToServer(m_serverName, QIODevice::WriteOnly);
    if (probe.waitForConnected(150)) {
        m_primary = false;
        return false;
    }

    QLocalServer::removeServer(m_serverName);
    m_primary = m_server->listen(m_serverName);
    return m_primary;
}

bool SingleInstance::isPrimary() const
{
    return m_primary;
}

bool SingleInstance::notifyPrimary(const QString &message, int timeoutMs) const
{
    QLocalSocket socket;
    socket.connectToServer(m_serverName, QIODevice::WriteOnly);
    if (!socket.waitForConnected(timeoutMs)) {
        return false;
    }
    socket.write(message.toUtf8());
    if (!socket.waitForBytesWritten(timeoutMs)) {
        return false;
    }
    socket.disconnectFromServer();
    return true;
}

void SingleInstance::acceptConnections()
{
    while (QLocalSocket *socket = m_server->nextPendingConnection()) {
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] {
            emit messageReceived(QString::fromUtf8(socket->readAll()));
        });
        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    }
}
