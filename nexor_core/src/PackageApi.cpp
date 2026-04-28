#include "PackageApi.h"
#include "Http.h"
#include "PackageRegistry.h"
#include "../../nexor_studio/src/build/PackageReader.h"
#include "../../nexor_studio/src/build/PackageDiff.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QFile>

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

QJsonObject auditToJson(const AuditEvent &e) {
    QJsonObject o;
    o.insert("id",          e.id);
    o.insert("event_type",  e.eventType);
    o.insert("package_id",  e.packageId);
    o.insert("version",     e.packageVersion);
    o.insert("actor",       e.actor);
    o.insert("detail",      e.detail);
    o.insert("occurred_at", e.occurredAt.toUTC().toString(Qt::ISODate));
    return o;
}

// Identity tag for the audit log: last 6 chars of the bearer token (so the
// log doesn't store the secret in clear), or "<anon>" when unauthenticated.
QString actorOf(const HttpRequest &req) {
    QString h = req.header("Authorization");
    QString prefix = "Bearer ";
    if (!h.startsWith(prefix, Qt::CaseInsensitive)) return "<anon>";
    QString tok = h.mid(prefix.size()).trimmed();
    if (tok.isEmpty()) return "<anon>";
    return tok.size() > 6 ? "…" + tok.right(6) : tok;
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
            if (!reg->publish(req.body, actorOf(req), rec, &err)) {
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
            if (!reg->deploy(id, ver, actorOf(req), &err)) {
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
            if (!reg->rollback(id, ver, actorOf(req), &err)) {
                res.setStatus(400, "Bad Request");
                res.setJson(jsonError(err));
                return;
            }
            RegistryRecord r;
            reg->find(id, ver, r);
            res.setJson(QJsonDocument(recordToJson(r))
                            .toJson(QJsonDocument::Compact));
        });

    // DELETE /api/v1/admin/packages/:id/:version
    //   200 on success; only allowed when the version is "pending".
    m_router->route("DELETE", "/api/v1/admin/packages/:id/:version",
        [reg = m_registry, tok = m_adminToken](const HttpRequest &req,
                                                HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QString id  = req.pathParams.value("id");
            QString ver = req.pathParams.value("version");
            QString err;
            if (!reg->deletePending(id, ver, actorOf(req), &err)) {
                res.setStatus(400, "Bad Request");
                res.setJson(jsonError(err));
                return;
            }
            QJsonObject o; o.insert("ok", true);
            o.insert("id", id); o.insert("version", ver);
            res.setJson(QJsonDocument(o).toJson(QJsonDocument::Compact));
        });

    // GET /api/v1/admin/packages/:id/history — every row for one project.
    m_router->route("GET", "/api/v1/admin/packages/:id/history",
        [reg = m_registry, tok = m_adminToken](const HttpRequest &req,
                                                HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QString id = req.pathParams.value("id");
            QJsonArray arr;
            for (const auto &r : reg->listByPackage(id)) arr.append(recordToJson(r));
            res.setJson(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        });

    // GET /api/v1/admin/audit[?package=…]
    m_router->route("GET", "/api/v1/admin/audit",
        [reg = m_registry, tok = m_adminToken](const HttpRequest &req,
                                                HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QUrlQuery qq(req.query);
            QString pkg = qq.queryItemValue("package");
            QJsonArray arr;
            for (const auto &e : reg->audit(pkg, 500)) arr.append(auditToJson(e));
            res.setJson(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        });

    // GET /api/v1/admin/packages/:id/diff?from=A&to=B
    m_router->route("GET", "/api/v1/admin/packages/:id/diff",
        [reg = m_registry, tok = m_adminToken](const HttpRequest &req,
                                                HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QString id = req.pathParams.value("id");
            QUrlQuery qq(req.query);
            QString fromV = qq.queryItemValue("from");
            QString toV   = qq.queryItemValue("to");
            if (fromV.isEmpty() || toV.isEmpty()) {
                res.setStatus(400, "Bad Request");
                res.setJson(jsonError("Both `from` and `to` query parameters are required."));
                return;
            }
            QByteArray fromBytes = reg->read(id, fromV);
            QByteArray toBytes   = reg->read(id, toV);
            if (fromBytes.isEmpty() || toBytes.isEmpty()) {
                res.setStatus(404, "Not Found");
                res.setJson(jsonError("One or both versions are missing."));
                return;
            }
            auto fromR = PackageReader::fromBytes(fromBytes, /*verifyHash*/false);
            auto toR   = PackageReader::fromBytes(toBytes,   /*verifyHash*/false);
            if (fromR.status != PackageReader::Status::Ok ||
                toR.status   != PackageReader::Status::Ok) {
                res.setStatus(500, "Internal Server Error");
                res.setJson(jsonError("Stored package failed to parse."));
                return;
            }
            PackageDiff diff = PackageDiffer::compute(fromR.package, toR.package);

            QJsonArray entries;
            for (const auto &c : diff.entries) {
                QJsonObject o;
                o.insert("kind",
                    c.kind == EntryChange::Added     ? "added" :
                    c.kind == EntryChange::Removed   ? "removed" :
                    c.kind == EntryChange::Changed   ? "changed" : "unchanged");
                o.insert("section",   c.section);
                o.insert("id",        c.id);
                o.insert("from_hash", c.fromHash);
                o.insert("to_hash",   c.toHash);
                entries.append(o);
            }
            QJsonArray sheets;
            for (const auto &s : diff.sheets) {
                QJsonArray fields;
                for (const auto &f : s.fields) {
                    QJsonObject fo;
                    fo.insert("kind",
                        f.kind == SheetFieldChange::Added        ? "added" :
                        f.kind == SheetFieldChange::Removed      ? "removed" :
                        f.kind == SheetFieldChange::TypeChanged  ? "type-changed"
                                                                  : "flags-changed");
                    fo.insert("name",      f.fieldName);
                    fo.insert("from_type", f.fromType);
                    fo.insert("to_type",   f.toType);
                    fo.insert("detail",    f.detail);
                    fields.append(fo);
                }
                QJsonObject so;
                so.insert("sheet_id", s.sheetId);
                so.insert("fields",   fields);
                sheets.append(so);
            }
            QJsonObject root;
            root.insert("from",    fromV);
            root.insert("to",      toV);
            root.insert("entries", entries);
            root.insert("sheets",  sheets);
            res.setJson(QJsonDocument(root).toJson(QJsonDocument::Compact));
        });
}

} // namespace nx
