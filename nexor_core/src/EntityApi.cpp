#include "EntityApi.h"
#include "Http.h"
#include "CoreEntityStore.h"
#include "ValueJson.h"
#include "../../nexor_studio/src/language/EntityStore.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace nx {

namespace {

QByteArray jsonError(const QString &msg) {
    QJsonObject o; o.insert("error", msg);
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

bool checkBearer(const HttpRequest &req, const QString &expected,
                 HttpResponse &res) {
    if (expected.isEmpty()) return true;
    QString got = req.header("Authorization");
    QString prefix = "Bearer ";
    if (!got.startsWith(prefix, Qt::CaseInsensitive)) {
        res.setStatus(401, "Unauthorized");
        res.setJson(jsonError("Missing or malformed Authorization header."));
        return false;
    }
    if (got.mid(prefix.size()).trimmed() != expected) {
        res.setStatus(401, "Unauthorized");
        res.setJson(jsonError("Invalid bearer token."));
        return false;
    }
    return true;
}

QJsonObject entityToJson(const std::shared_ptr<Entity> &e) {
    QJsonObject o;
    if (!e) return o;
    o.insert("id",     static_cast<double>(e->id()));
    o.insert("$sheet", e->sheetId());
    QJsonObject f;
    for (const auto &name : e->fieldNames())
        f.insert(name, valueToJson(e->get(name)));
    o.insert("fields", f);
    return o;
}

// Accept either {fields: {...}} or {...} as the body shape, for ergonomics.
QJsonObject extractFields(const QByteArray &body) {
    QJsonObject root = QJsonDocument::fromJson(body).object();
    if (root.contains("fields") && root.value("fields").isObject())
        return root.value("fields").toObject();
    return root;
}

void applyFieldsToEntity(const std::shared_ptr<Entity> &e,
                         const QJsonObject &fields) {
    if (!e) return;
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it)
        e->set(it.key(), jsonToValue(it.value()));
}

} // namespace

EntityApi::EntityApi(Router *router, CoreEntityStore *store)
    : m_router(router), m_store(store) {}

void EntityApi::registerRoutes() {

    // GET /api/v1/entities/:sheet
    m_router->route("GET", "/api/v1/entities/:sheet",
        [store = m_store, tok = m_adminToken](const HttpRequest &req,
                                               HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QString sheet = req.pathParams.value("sheet");
            EntityTable *t = store->inner()->table(sheet);
            if (!t) {
                res.setStatus(404, "Not Found");
                res.setJson(jsonError("No such sheet: " + sheet));
                return;
            }
            QJsonArray arr;
            for (const auto &e : t->all()) arr.append(entityToJson(e));
            res.setJson(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        });

    // GET /api/v1/entities/:sheet/:id
    m_router->route("GET", "/api/v1/entities/:sheet/:id",
        [store = m_store, tok = m_adminToken](const HttpRequest &req,
                                               HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QString sheet = req.pathParams.value("sheet");
            qint64 id     = req.pathParams.value("id").toLongLong();
            EntityTable *t = store->inner()->table(sheet);
            if (!t) { res.setStatus(404, "Not Found");
                     res.setJson(jsonError("No such sheet: " + sheet)); return; }
            auto e = t->find(id);
            if (!e) { res.setStatus(404, "Not Found");
                     res.setJson(jsonError(QString("No %1 with id %2").arg(sheet)
                                                                       .arg(id)));
                     return; }
            res.setJson(QJsonDocument(entityToJson(e)).toJson(QJsonDocument::Compact));
        });

    // POST /api/v1/entities/:sheet
    m_router->route("POST", "/api/v1/entities/:sheet",
        [store = m_store, tok = m_adminToken](const HttpRequest &req,
                                               HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QString sheet = req.pathParams.value("sheet");
            EntityTable *t = store->inner()->table(sheet);
            if (!t) { res.setStatus(404, "Not Found");
                     res.setJson(jsonError("No such sheet: " + sheet)); return; }
            auto e = t->create();
            applyFieldsToEntity(e, extractFields(req.body));
            if (!t->save(e)) {
                res.setStatus(500, "Internal Server Error");
                res.setJson(jsonError("Save failed."));
                return;
            }
            res.setStatus(201, "Created");
            res.setJson(QJsonDocument(entityToJson(e)).toJson(QJsonDocument::Compact));
        });

    // PATCH /api/v1/entities/:sheet/:id
    m_router->route("PATCH", "/api/v1/entities/:sheet/:id",
        [store = m_store, tok = m_adminToken](const HttpRequest &req,
                                               HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QString sheet = req.pathParams.value("sheet");
            qint64 id     = req.pathParams.value("id").toLongLong();
            EntityTable *t = store->inner()->table(sheet);
            if (!t) { res.setStatus(404, "Not Found");
                     res.setJson(jsonError("No such sheet: " + sheet)); return; }
            auto e = t->find(id);
            if (!e) { res.setStatus(404, "Not Found");
                     res.setJson(jsonError(QString("No %1 with id %2").arg(sheet)
                                                                       .arg(id)));
                     return; }
            applyFieldsToEntity(e, extractFields(req.body));
            if (!t->save(e)) {
                res.setStatus(500, "Internal Server Error");
                res.setJson(jsonError("Save failed."));
                return;
            }
            res.setJson(QJsonDocument(entityToJson(e)).toJson(QJsonDocument::Compact));
        });

    // DELETE /api/v1/entities/:sheet/:id
    m_router->route("DELETE", "/api/v1/entities/:sheet/:id",
        [store = m_store, tok = m_adminToken](const HttpRequest &req,
                                               HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QString sheet = req.pathParams.value("sheet");
            qint64 id     = req.pathParams.value("id").toLongLong();
            EntityTable *t = store->inner()->table(sheet);
            if (!t) { res.setStatus(404, "Not Found");
                     res.setJson(jsonError("No such sheet: " + sheet)); return; }
            bool ok = t->remove(id);
            QJsonObject o;
            o.insert("ok", ok);
            o.insert("id", static_cast<double>(id));
            res.setJson(QJsonDocument(o).toJson(QJsonDocument::Compact));
        });
}

} // namespace nx
