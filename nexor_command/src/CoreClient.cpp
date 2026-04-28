#include "CoreClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>

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

void CoreClient::registerPackage(const QByteArray &bytes, const QString &localPath) {
    QNetworkRequest req(QUrl(m_baseUrl + "/api/v1/packages"));
    if (!m_token.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  "application/x-nexor-package");
    req.setRawHeader("Accept", "application/json");
    QNetworkReply *r = m_nam->post(req, bytes);
    QString hint = localPath.isEmpty() ? QString("(in-memory bytes)")
                                       : QFileInfo(localPath).fileName();
    connect(r, &QNetworkReply::finished, this, [this, r, hint]{
        QByteArray body = r->readAll();
        bool ok = (r->error() == QNetworkReply::NoError);
        QString summary = ok
            ? QString("register %1 OK").arg(hint)
            : QString("register %1 FAILED: %2").arg(hint, r->errorString());
        if (!body.isEmpty()) summary += "  " + QString::fromUtf8(body);
        emit operationFinished("register", ok, summary);
        r->deleteLater();
    });
}

QNetworkReply *CoreClient::deleteRq(const QString &path, bool authed) {
    QNetworkRequest req(QUrl(m_baseUrl + path));
    if (authed && !m_token.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    req.setRawHeader("Accept", "application/json");
    return m_nam->deleteResource(req);
}

AuditRow CoreClient::auditFromJson(const QJsonObject &o) {
    AuditRow r;
    r.id         = static_cast<qint64>(o.value("id").toDouble());
    r.eventType  = o.value("event_type").toString();
    r.packageId  = o.value("package_id").toString();
    r.version    = o.value("version").toString();
    r.actor      = o.value("actor").toString();
    r.detail     = o.value("detail").toString();
    r.occurredAt = o.value("occurred_at").toString();
    return r;
}

DiffResult CoreClient::diffFromJson(const QJsonDocument &doc) {
    DiffResult d;
    QJsonObject root = doc.object();
    d.fromVersion = root.value("from").toString();
    d.toVersion   = root.value("to").toString();
    for (const auto &v : root.value("entries").toArray()) {
        QJsonObject o = v.toObject();
        DiffEntry e;
        e.kind     = o.value("kind").toString();
        e.section  = o.value("section").toString();
        e.id       = o.value("id").toString();
        e.fromHash = o.value("from_hash").toString();
        e.toHash   = o.value("to_hash").toString();
        d.entries.append(e);
    }
    for (const auto &v : root.value("sheets").toArray()) {
        QJsonObject so = v.toObject();
        SheetDiff sd;
        sd.sheetId = so.value("sheet_id").toString();
        for (const auto &fv : so.value("fields").toArray()) {
            QJsonObject fo = fv.toObject();
            SheetFieldDiff f;
            f.kind     = fo.value("kind").toString();
            f.name     = fo.value("name").toString();
            f.fromType = fo.value("from_type").toString();
            f.toType   = fo.value("to_type").toString();
            f.detail   = fo.value("detail").toString();
            sd.fields.append(f);
        }
        d.sheets.append(sd);
    }
    return d;
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

void CoreClient::deletePending(const QString &id, const QString &version) {
    QString path = "/api/v1/admin/packages/" + id + "/" + version;
    wireOpReply(deleteRq(path, /*authed*/true), this, "delete-pending", id, version);
}

void CoreClient::historyOf(const QString &id) {
    QNetworkReply *r = get("/api/v1/admin/packages/" + id + "/history",
                           /*authed*/true);
    connect(r, &QNetworkReply::finished, this, [this, r, id]{
        QByteArray body = r->readAll();
        QVector<PackageRow> rows;
        if (r->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(body);
            if (doc.isArray())
                for (const auto &v : doc.array())
                    rows.append(rowFromJson(v.toObject()));
        } else {
            emit operationFinished("history", false,
                "history failed: " + r->errorString());
        }
        emit historyReceived(id, rows);
        r->deleteLater();
    });
}

void CoreClient::auditLog(const QString &packageId) {
    QString path = "/api/v1/admin/audit";
    if (!packageId.isEmpty()) path += "?package=" + packageId;
    QNetworkReply *r = get(path, /*authed*/true);
    connect(r, &QNetworkReply::finished, this, [this, r]{
        QByteArray body = r->readAll();
        QVector<AuditRow> rows;
        if (r->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(body);
            if (doc.isArray())
                for (const auto &v : doc.array())
                    rows.append(auditFromJson(v.toObject()));
        } else {
            emit operationFinished("audit", false,
                "audit failed: " + r->errorString());
        }
        emit auditReceived(rows);
        r->deleteLater();
    });
}

void CoreClient::diff(const QString &id, const QString &from, const QString &to) {
    QString path = QString("/api/v1/admin/packages/%1/diff?from=%2&to=%3")
                       .arg(id, from, to);
    QNetworkReply *r = get(path, /*authed*/true);
    connect(r, &QNetworkReply::finished, this, [this, r]{
        QByteArray body = r->readAll();
        if (r->error() != QNetworkReply::NoError) {
            emit operationFinished("diff", false, "diff failed: " + r->errorString());
            r->deleteLater();
            return;
        }
        emit diffReceived(diffFromJson(QJsonDocument::fromJson(body)));
        r->deleteLater();
    });
}

void CoreClient::downloadTo(const QString &id, const QString &version,
                            const QString &localPath) {
    QNetworkReply *r = get("/api/v1/packages/" + id + "/" + version,
                           /*authed*/false);
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
