#include "CoreClient.h"
#include <QTcpSocket>

CoreClient::CoreClient(QObject *parent)
    : QObject(parent), m_sock(new QTcpSocket(this)) {
    connect(m_sock, &QTcpSocket::connected,    this, &CoreClient::connected);
    connect(m_sock, &QTcpSocket::disconnected, this, &CoreClient::disconnected);
    connect(m_sock, &QTcpSocket::readyRead,    this, &CoreClient::onReadyRead);
    connect(m_sock, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &CoreClient::onSocketError);
}

CoreClient::~CoreClient() = default;

void CoreClient::connectToCore(const QString &host, quint16 port) {
    if (m_sock->state() != QAbstractSocket::UnconnectedState)
        m_sock->abort();
    m_sock->connectToHost(host, port);
}

void CoreClient::disconnectFromCore() {
    m_sock->disconnectFromHost();
}

bool CoreClient::isConnected() const {
    return m_sock->state() == QAbstractSocket::ConnectedState;
}

void CoreClient::send(const QByteArray &payload) {
    if (isConnected()) m_sock->write(payload);
}

void CoreClient::onReadyRead() {
    emit messageReceived(m_sock->readAll());
}

void CoreClient::onSocketError() {
    emit errorOccurred(m_sock->errorString());
}
