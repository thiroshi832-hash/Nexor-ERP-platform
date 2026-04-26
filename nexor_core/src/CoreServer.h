// =============================================================================
// CoreServer — TCP listener that hosts ClientSession objects.
// =============================================================================
#ifndef NEXOR_CORE_COREServer_H
#define NEXOR_CORE_COREServer_H

#include <QObject>
#include <QTcpServer>
#include <QHash>

class ClientSession;

class CoreServer : public QObject {
    Q_OBJECT
public:
    explicit CoreServer(QObject *parent = nullptr);
    ~CoreServer() override;

    bool start(quint16 port);
    void stop();

    int sessionCount() const { return m_sessions.size(); }

private slots:
    void onNewConnection();
    void onSessionClosed();

private:
    QTcpServer *m_server;
    QHash<QObject*, ClientSession*> m_sessions;
};

#endif // NEXOR_CORE_COREServer_H
