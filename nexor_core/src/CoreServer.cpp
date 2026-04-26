#include "CoreServer.h"
#include "ClientSession.h"

#include <QTcpSocket>
#include <QDebug>

CoreServer::CoreServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this)) {
    connect(m_server, &QTcpServer::newConnection, this, &CoreServer::onNewConnection);
}

CoreServer::~CoreServer() = default;

bool CoreServer::start(quint16 port) {
    return m_server->listen(QHostAddress::Any, port);
}

void CoreServer::stop() {
    m_server->close();
    for (auto *s : m_sessions) s->deleteLater();
    m_sessions.clear();
}

void CoreServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket *sock = m_server->nextPendingConnection();
        auto *session = new ClientSession(sock, this);
        m_sessions.insert(session, session);
        connect(session, &ClientSession::closed, this, &CoreServer::onSessionClosed);
        qInfo() << "New session from" << sock->peerAddress().toString()
                << "(" << m_sessions.size() << "active)";
    }
}

void CoreServer::onSessionClosed() {
    auto *session = qobject_cast<ClientSession*>(sender());
    if (!session) return;
    m_sessions.remove(session);
    session->deleteLater();
    qInfo() << "Session closed (" << m_sessions.size() << "active)";
}
