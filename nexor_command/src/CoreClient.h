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

struct AuditRow {
    qint64  id { 0 };
    QString eventType;
    QString packageId, version;
    QString actor;
    QString detail;
    QString occurredAt;
};

struct DiffEntry {
    QString kind;          // "added" | "removed" | "changed" | "unchanged"
    QString section;
    QString id;
    QString fromHash, toHash;
};

struct SheetFieldDiff {
    QString kind;          // "added" | "removed" | "type-changed" | "flags-changed"
    QString name, fromType, toType, detail;
};

struct SheetDiff {
    QString sheetId;
    QVector<SheetFieldDiff> fields;
};

struct DiffResult {
    QString fromVersion, toVersion;
    QVector<DiffEntry> entries;
    QVector<SheetDiff> sheets;
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
    // Uploads `bytes` (a .nexor file's raw contents) to Core's
    // /api/v1/packages endpoint.  This is Command's job - Studio never
    // talks to Core directly; it writes the package to disk and an admin
    // running Command pushes it through.
    void registerPackage(const QByteArray &bytes,
                         const QString &localPath = QString());
    void deploy        (const QString &id, const QString &version);
    void rollback      (const QString &id, const QString &version);
    void deletePending (const QString &id, const QString &version);
    void historyOf     (const QString &id);
    void auditLog      (const QString &packageId = QString());
    void diff          (const QString &id, const QString &from, const QString &to);
    void downloadTo    (const QString &id, const QString &version,
                        const QString &localPath);
    void checkHealth   ();

signals:
    void packagesReceived (const QVector<PackageRow> &rows);
    void historyReceived  (const QString &id, const QVector<PackageRow> &rows);
    void auditReceived    (const QVector<AuditRow> &events);
    void diffReceived     (const DiffResult &diff);
    void downloadFinished (const QString &id, const QString &version,
                           bool ok, const QString &localPath, const QString &message);
    void operationFinished(const QString &op, bool ok, const QString &message);
    void healthReceived   (bool reachable, const QString &info);

private:
    QNetworkAccessManager *m_nam;
    QString                m_baseUrl;
    QString                m_token;

    QNetworkReply *get      (const QString &path, bool authed);
    QNetworkReply *post     (const QString &path, bool authed,
                             const QByteArray &body = {},
                             const QString &contentType = "application/json");
    QNetworkReply *deleteRq (const QString &path, bool authed);
    static PackageRow rowFromJson  (const QJsonObject &o);
    static AuditRow   auditFromJson(const QJsonObject &o);
    static DiffResult diffFromJson (const QJsonDocument &doc);
};

} // namespace nx

#endif // NEXOR_COMMAND_COREClient_H
