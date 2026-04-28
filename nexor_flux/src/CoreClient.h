// =============================================================================
// CoreClient — Flux's HTTP client for talking to Nexor Core.
//
// Uses the public, anonymous endpoints (Phase 9):
//   GET /api/v1/health
//   GET /api/v1/packages                       -- catalog
//   GET /api/v1/packages/:id/:version          -- download .nexor bytes
//
// No bearer token required; Flux is the end-user-facing host.  Studio +
// Command are the auth'd clients.
//
// Phase 13 ships this single client; Phase 13b adds the WebSocket
// subscriber for live update notifications (package-published).
// =============================================================================
#ifndef NEXOR_FLUX_COREClient_H
#define NEXOR_FLUX_COREClient_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVector>
#include <QJsonObject>

class QNetworkAccessManager;
class QNetworkReply;

namespace nx {

struct CatalogRow {
    QString id, version, title, hash, hashAlgo, status;
    QString builtAt, receivedAt;
    qint64  byteSize { 0 };
};

class CoreClient : public QObject {
    Q_OBJECT
public:
    explicit CoreClient(QObject *parent = nullptr);

    void setBaseUrl(const QString &url) { m_baseUrl = url; }
    QString baseUrl() const             { return m_baseUrl; }

    void checkHealth();
    void listCatalog();                                      // public listing
    void downloadPackage(const QString &id, const QString &version,
                         const QString &localPath);

signals:
    void healthReceived (bool reachable, const QString &info);
    void catalogReceived(const QVector<CatalogRow> &rows);
    void downloadFinished(const QString &id, const QString &version,
                          bool ok, const QString &localPath, const QString &message);

private:
    QNetworkAccessManager *m_nam;
    QString                m_baseUrl;

    QNetworkReply *get(const QString &path);
    static CatalogRow rowFromJson(const QJsonObject &o);
};

} // namespace nx

#endif // NEXOR_FLUX_COREClient_H
