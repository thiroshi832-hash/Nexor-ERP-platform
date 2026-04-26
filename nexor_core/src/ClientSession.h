// =============================================================================
// ClientSession — one connected Nexor Flux client.
// =============================================================================
#ifndef NEXOR_CORE_CLIENTSESSION_H
#define NEXOR_CORE_CLIENTSESSION_H

#include <QObject>
#include <QByteArray>

class QTcpSocket;

class ClientSession : public QObject {
    Q_OBJECT
public:
    explicit ClientSession(QTcpSocket *sock, QObject *parent = nullptr);
    ~ClientSession() override;

signals:
    void closed();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket *m_sock;
    QByteArray  m_buffer;
};

#endif // NEXOR_CORE_CLIENTSESSION_H
