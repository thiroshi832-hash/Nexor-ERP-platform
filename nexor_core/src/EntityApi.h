// =============================================================================
// EntityApi — registers the /api/v1/entities/:sheet[/:id] family.
//
//   GET    /api/v1/entities/:sheet                 -> [{id, ...fields}]
//   GET    /api/v1/entities/:sheet/:id              -> {id, ...fields}
//   POST   /api/v1/entities/:sheet                  -> {id, ...fields}
//          body = {fields: {...}}                       (or {...} directly)
//   PATCH  /api/v1/entities/:sheet/:id               -> {id, ...fields}
//          body = {fields: {...}}                       partial update
//   DELETE /api/v1/entities/:sheet/:id               -> {ok:true, id}
//
// All endpoints require bearer auth (same admin-token Core uses for
// publish + admin/*).  Phase 15c will split tenant + per-user tokens.
// =============================================================================
#ifndef NEXOR_CORE_ENTITYAPI_H
#define NEXOR_CORE_ENTITYAPI_H

#include <QString>

namespace nx {

class Router;
class CoreEntityStore;

class EntityApi {
public:
    EntityApi(Router *router, CoreEntityStore *store);

    void setAdminToken(const QString &t) { m_adminToken = t; }
    void registerRoutes();

private:
    Router          *m_router;
    CoreEntityStore *m_store;
    QString          m_adminToken;
};

} // namespace nx

#endif // NEXOR_CORE_ENTITYAPI_H
