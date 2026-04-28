#include "PackageApi.h"
#include "Http.h"
#include "PackageRegistry.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace nx {

namespace {

QByteArray jsonError(const QString &msg) {
    QJsonObject o; o.insert("error", msg);
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

QJsonObject recordToJson(const RegistryRecord &r) {
    QJsonObject o;
    o.insert("id",          r.id);
    o.insert("version",     r.version);
    o.insert("title",       r.title);
    o.insert("built_at",    r.builtAt.toUTC().toString(Qt::ISODate));
    o.insert("received_at", r.receivedAt.toUTC().toString(Qt::ISODate));
    o.insert("hash",        r.hash);
    o.insert("hash_algo",   r.hashAlgo);
    o.insert("byte_size",   static_cast<qint64>(r.byteSize));
    o.insert("status",      r.status);
    return o;
}

} // namespace

PackageApi::PackageApi(Router *router, PackageRegistry *registry)
    : m_router(router), m_registry(registry),
      m_startedAt(QDateTime::currentDateTimeUtc()) {}

void PackageApi::registerRoutes() {
    // Health — sanity probe + cheap "yes the build linked" check.
    m_router->route("GET", "/api/v1/health",
        [started = m_startedAt](const HttpRequest&, HttpResponse &res) {
            QJsonObject o;
            o.insert("service", "NexorCore");
            o.insert("version", "0.1.0");
            o.insert("uptime",
                started.secsTo(QDateTime::currentDateTimeUtc()));
            res.setJson(QJsonDocument(o).toJson(QJsonDocument::Compact));
        });

    // POST /api/v1/packages — receive a .nexor.
    m_router->route("POST", "/api/v1/packages",
        [reg = m_registry](const HttpRequest &req, HttpResponse &res) {
            if (req.body.isEmpty()) {
                res.setStatus(400, "Bad Request");
                res.setJson(jsonError("Empty body."));
                return;
            }
            RegistryRecord rec;
            QString err;
            if (!reg->publish(req.body, rec, &err)) {
                res.setStatus(400, "Bad Request");
                res.setJson(jsonError(err));
                return;
            }
            res.setJson(QJsonDocument(recordToJson(rec))
                            .toJson(QJsonDocument::Compact));
        });

    // GET /api/v1/packages — list everything.
    m_router->route("GET", "/api/v1/packages",
        [reg = m_registry](const HttpRequest&, HttpResponse &res) {
            QJsonArray arr;
            for (const auto &r : reg->list()) arr.append(recordToJson(r));
            res.setJson(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        });

    // GET /api/v1/packages/:id/:version — download the bytes.
    m_router->route("GET", "/api/v1/packages/:id/:version",
        [reg = m_registry](const HttpRequest &req, HttpResponse &res) {
            QString id  = req.pathParams.value("id");
            QString ver = req.pathParams.value("version");
            QByteArray bytes = reg->read(id, ver);
            if (bytes.isEmpty()) {
                res.setStatus(404, "Not Found");
                res.setJson(jsonError(
                    QString("Package %1/%2 not found.").arg(id, ver)));
                return;
            }
            res.setBody(bytes, "application/x-nexor-package");
        });
}

} // namespace nx
