#include "CoreClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

namespace nx {

CoreClient::CoreClient(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)) {}

PackageRow CoreClient::rowFromJson(const QJsonObject &o) {
    PackageRow r;
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

QNetworkReply *CoreClient::get(const QString &path, bool authed) {
    QNetworkRequest req(QUrl(m_baseUrl + path));
    if (authed && !m_token.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    req.setRawHeader("Accept", "application/json");
    return m_nam->get(req);
}

QNetworkReply *CoreClient::post(const QString &path, bool authed,
                                const QByteArray &body, const QString &ct) {
    QNetworkRequest req(QUrl(m_baseUrl + path));
    if (authed && !m_token.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, ct);
    req.setRawHeader("Accept", "application/json");
    return m_nam->post(req, body);
}

void CoreClient::checkHealth() {
    QNetworkReply *r = get("/api/v1/health", /*authed*/false);
    connect(r, &QNetworkReply::finished, this, [this, r]{
        bool ok = (r->error() == QNetworkReply::NoError);
        emit healthReceived(ok, ok ? QString::fromUtf8(r->readAll())
                                   : r->errorString());
        r->deleteLater();
    });
}

void CoreClient::listPackages(const QString &statusFilter) {
    QString path = "/api/v1/admin/packages";
    if (!statusFilter.isEmpty()) path += "?status=" + statusFilter;
    QNetworkReply *r = get(path, /*authed*/true);
    connect(r, &QNetworkReply::finished, this, [this, r]{
        QByteArray body = r->readAll();
        if (r->error() != QNetworkReply::NoError) {
            emit operationFinished("list", false,
                "list failed: " + r->errorString() +
                (body.isEmpty() ? QString() : (" — " + QString::fromUtf8(body))));
            r->deleteLater();
            return;
        }
        QVector<PackageRow> rows;
        QJsonDocument doc = QJsonDocument::fromJson(body);
        if (doc.isArray()) {
            for (const auto &v : doc.array()) rows.append(rowFromJson(v.toObject()));
        }
        emit packagesReceived(rows);
        r->deleteLater();
    });
}

static void wireOpReply(QNetworkReply *r, CoreClient *self,
                        const QString &op, const QString &id,
                        const QString &version) {
    QObject::connect(r, &QNetworkReply::finished, self, [r, self, op, id, version]{
        QByteArray body = r->readAll();
        bool ok = (r->error() == QNetworkReply::NoError);
        QString summary = ok
            ? QString("%1 %2 v%3 OK").arg(op, id, version)
            : QString("%1 %2 v%3 FAILED: %4").arg(op, id, version, r->errorString());
        if (!body.isEmpty()) summary += "  " + QString::fromUtf8(body);
        emit self->operationFinished(op, ok, summary);
        r->deleteLater();
    });
}

void CoreClient::deploy(const QString &id, const QString &version) {
    QString path = "/api/v1/admin/packages/" + id + "/" + version + "/deploy";
    wireOpReply(post(path, /*authed*/true), this, "deploy", id, version);
}

void CoreClient::rollback(const QString &id, const QString &version) {
    QString path = "/api/v1/admin/packages/" + id + "/" + version + "/rollback";
    wireOpReply(post(path, /*authed*/true), this, "rollback", id, version);
}

} // namespace nx
