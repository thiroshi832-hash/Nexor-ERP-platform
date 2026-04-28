#include "PackageApi.h"
#include "Http.h"
#include "PackageRegistry.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

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

// Bearer-token check.  Returns true when the request carries a matching
// Authorization: Bearer <token>.  An empty configured token short-circuits
// to "allow" so Core can start in permissive mode.
bool checkBearer(const HttpRequest &req, const QString &expected,
                 HttpResponse &res) {
    if (expected.isEmpty()) return true;
    QString got = req.header("Authorization");
    QString prefix = "Bearer ";
    if (!got.startsWith(prefix, Qt::CaseInsensitive)) {
        res.setStatus(401, "Unauthorized");
        res.headers.insert("WWW-Authenticate", "Bearer realm=\"NexorCore\"");
        res.setJson(jsonError("Missing or malformed Authorization header."));
        return false;
    }
    if (got.mid(prefix.size()).trimmed() != expected) {
        res.setStatus(401, "Unauthorized");
        res.headers.insert("WWW-Authenticate", "Bearer realm=\"NexorCore\"");
        res.setJson(jsonError("Invalid bearer token."));
        return false;
    }
    return true;
}

} // namespace

PackageApi::PackageApi(Router *router, PackageRegistry *registry)
    : m_router(router), m_registry(registry),
      m_startedAt(QDateTime::currentDateTimeUtc()) {}

void PackageApi::registerRoutes() {
    // Health — sanity probe.  Always open.
    m_router->route("GET", "/api/v1/health",
        [started = m_startedAt](const HttpRequest&, HttpResponse &res) {
            QJsonObject o;
            o.insert("service", "NexorCore");
            o.insert("version", "0.1.0");
            o.insert("uptime",
                started.secsTo(QDateTime::currentDateTimeUtc()));
            res.setJson(QJsonDocument(o).toJson(QJsonDocument::Compact));
        });

    // POST /api/v1/packages — receive a .nexor.  Bearer-protected.
    m_router->route("POST", "/api/v1/packages",
        [reg = m_registry, tok = m_adminToken](const HttpRequest &req,
                                                HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
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

    // GET /api/v1/packages — list everything (open).
    m_router->route("GET", "/api/v1/packages",
        [reg = m_registry](const HttpRequest&, HttpResponse &res) {
            QJsonArray arr;
            for (const auto &r : reg->list()) arr.append(recordToJson(r));
            res.setJson(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        });

    // GET /api/v1/packages/:id/:version — download the bytes (open).
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

    // GET /api/v1/admin/packages[?status=pending|live|rolled_back]
    m_router->route("GET", "/api/v1/admin/packages",
        [reg = m_registry, tok = m_adminToken](const HttpRequest &req,
                                                HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QUrlQuery qq(req.query);
            QString filter = qq.queryItemValue("status");
            QJsonArray arr;
            for (const auto &r : reg->list(filter)) arr.append(recordToJson(r));
            res.setJson(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        });

    // POST /api/v1/admin/packages/:id/:version/deploy
    m_router->route("POST", "/api/v1/admin/packages/:id/:version/deploy",
        [reg = m_registry, tok = m_adminToken](const HttpRequest &req,
                                                HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QString id  = req.pathParams.value("id");
            QString ver = req.pathParams.value("version");
            QString err;
            if (!reg->deploy(id, ver, &err)) {
                res.setStatus(400, "Bad Request");
                res.setJson(jsonError(err));
                return;
            }
            RegistryRecord r;
            reg->find(id, ver, r);
            res.setJson(QJsonDocument(recordToJson(r))
                            .toJson(QJsonDocument::Compact));
        });

    // POST /api/v1/admin/packages/:id/:version/rollback
    m_router->route("POST", "/api/v1/admin/packages/:id/:version/rollback",
        [reg = m_registry, tok = m_adminToken](const HttpRequest &req,
                                                HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QString id  = req.pathParams.value("id");
            QString ver = req.pathParams.value("version");
            QString err;
            if (!reg->rollback(id, ver, &err)) {
                res.setStatus(400, "Bad Request");
                res.setJson(jsonError(err));
                return;
            }
            RegistryRecord r;
            reg->find(id, ver, r);
            res.setJson(QJsonDocument(recordToJson(r))
                            .toJson(QJsonDocument::Compact));
        });
}

} // namespace nx
