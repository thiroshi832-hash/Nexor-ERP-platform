#include "ClientSession.h"
#include <QTcpSocket>
#include <QDebug>

ClientSession::ClientSession(QTcpSocket *sock, QObject *parent)
    : QObject(parent), m_sock(sock) {
    m_sock->setParent(this);
    connect(m_sock, &QTcpSocket::readyRead,    this, &ClientSession::onReadyRead);
    connect(m_sock, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);
}

ClientSession::~ClientSession() = default;

void ClientSession::onReadyRead() {
    m_buffer.append(m_sock->readAll());
    // TODO: framing & dispatch — placeholder echoes for now.
    if (!m_buffer.isEmpty()) {
        m_sock->write(m_buffer);
        m_buffer.clear();
    }
}

void ClientSession::onDisconnected() {
    emit closed();
}
