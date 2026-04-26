// =============================================================================
// CoreClient — TCP client that talks to NexorCore.
// =============================================================================
#ifndef NEXOR_FLUX_COREClient_H
#define NEXOR_FLUX_COREClient_H

#include <QObject>
#include <QByteArray>

class QTcpSocket;

class CoreClient : public QObject {
    Q_OBJECT
public:
    explicit CoreClient(QObject *parent = nullptr);
    ~CoreClient() override;

    void connectToCore(const QString &host, quint16 port);
    void disconnectFromCore();
    bool isConnected() const;

    void send(const QByteArray &payload);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QByteArray &payload);
    void errorOccurred(const QString &what);

private slots:
    void onReadyRead();
    void onSocketError();

private:
    QTcpSocket *m_sock;
};

#endif // NEXOR_FLUX_COREClient_H
