// =============================================================================
// CoreClient — thin wrapper around the four Nexor Core admin calls Command
// uses today.  Keeps QNetworkAccessManager + Authorization-header fiddling
// out of the UI layer.
//
//   listPackages(statusFilter)     -> GET  /api/v1/admin/packages[?status=…]
//   deploy   (id, version)         -> POST /api/v1/admin/packages/:id/:version/deploy
//   rollback (id, version)         -> POST /api/v1/admin/packages/:id/:version/rollback
//   health()                       -> GET  /api/v1/health
//
// Each call emits exactly one signal (success or failure) when it returns.
// =============================================================================
#ifndef NEXOR_COMMAND_COREClient_H
#define NEXOR_COMMAND_COREClient_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>

class QNetworkAccessManager;
class QNetworkReply;

namespace nx {

struct PackageRow {
    QString id, version, title, hash, hashAlgo, status;
    QString builtAt, receivedAt;
    qint64  byteSize { 0 };
};

class CoreClient : public QObject {
    Q_OBJECT
public:
    explicit CoreClient(QObject *parent = nullptr);

    void setBaseUrl   (const QString &url)   { m_baseUrl = url; }
    void setAdminToken(const QString &token) { m_token   = token; }

    QString baseUrl()    const { return m_baseUrl; }
    QString adminToken() const { return m_token; }

    void listPackages(const QString &statusFilter = QString());
    void deploy      (const QString &id, const QString &version);
    void rollback    (const QString &id, const QString &version);
    void checkHealth ();

signals:
    void packagesReceived(const QVector<PackageRow> &rows);
    void operationFinished(const QString &op, bool ok, const QString &message);
    void healthReceived  (bool reachable, const QString &info);

private:
    QNetworkAccessManager *m_nam;
    QString                m_baseUrl;
    QString                m_token;

    QNetworkReply *get  (const QString &path, bool authed);
    QNetworkReply *post (const QString &path, bool authed,
                         const QByteArray &body = {},
                         const QString &contentType = "application/json");
    static PackageRow rowFromJson(const QJsonObject &o);
};

} // namespace nx

#endif // NEXOR_COMMAND_COREClient_H
