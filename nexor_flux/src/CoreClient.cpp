#include "CoreClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QUrl>

namespace nx {

CoreClient::CoreClient(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)) {}

CatalogRow CoreClient::rowFromJson(const QJsonObject &o) {
    CatalogRow r;
    r.id         = o.value("id").toString();
    r.version    = o.value("version").toString();
    r.title      = o.value("title").toString();
    r.hash       = o.value("hash").toString();
    r.hashAlgo   = o.value("hash_algo").toString();
    r.status     = o.value("status").toString();
    r.builtAt    = o.value("built_at").toString();
    r.receivedAt = o.value("received_at").toString();
    r.byteSize   = static_cast<qint64>(o.value("byte_size").toDouble());
    return r;
}

QNetworkReply *CoreClient::get(const QString &path) {
    QNetworkRequest req(QUrl(m_baseUrl + path));
    req.setRawHeader("Accept", "application/json");
    return m_nam->get(req);
}

void CoreClient::checkHealth() {
    QNetworkReply *r = get("/api/v1/health");
    connect(r, &QNetworkReply::finished, this, [this, r]{
        bool ok = (r->error() == QNetworkReply::NoError);
        emit healthReceived(ok, ok ? QString::fromUtf8(r->readAll())
                                   : r->errorString());
        r->deleteLater();
    });
}

void CoreClient::listCatalog() {
    QNetworkReply *r = get("/api/v1/packages");
    connect(r, &QNetworkReply::finished, this, [this, r]{
        QVector<CatalogRow> rows;
        if (r->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(r->readAll());
            if (doc.isArray())
                for (const auto &v : doc.array())
                    rows.append(rowFromJson(v.toObject()));
        }
        emit catalogReceived(rows);
        r->deleteLater();
    });
}

void CoreClient::downloadPackage(const QString &id, const QString &version,
                                 const QString &localPath) {
    QNetworkReply *r = get("/api/v1/packages/" + id + "/" + version);
    connect(r, &QNetworkReply::finished, this, [this, r, id, version, localPath]{
        if (r->error() != QNetworkReply::NoError) {
            emit downloadFinished(id, version, false, localPath,
                "download failed: " + r->errorString());
            r->deleteLater();
            return;
        }
        QDir().mkpath(QFileInfo(localPath).absolutePath());
        QFile f(localPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit downloadFinished(id, version, false, localPath,
                "cannot write " + localPath);
            r->deleteLater();
            return;
        }
        f.write(r->readAll());
        f.close();
        emit downloadFinished(id, version, true, localPath,
            "saved to " + localPath);
        r->deleteLater();
    });
}

} // namespace nx
