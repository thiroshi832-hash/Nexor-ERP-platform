// =============================================================================
// PackageApi — registers /api/v1/packages, /api/v1/admin/* and /api/v1/health
// against a Router, backed by a PackageRegistry.
//
// Endpoints (Phase 9 + 9c):
//
//   GET  /api/v1/health
//        Body : { "service": "NexorCore", "version": "0.1.0", "uptime": <s> }
//
//   POST /api/v1/packages                                     [auth: bearer]
//        Body : raw .nexor bytes  (Content-Type: application/x-nexor-package
//                                  or application/octet-stream)
//        200  : { "id":"…", "version":"…", "hash":"…", "status":"pending" }
//        400  : { "error": "…" }                              (parse / hash / sig)
//        401  : { "error": "…" }                              (missing/bad token)
//
//   GET  /api/v1/packages
//        200  : [ { id, version, title, built_at, received_at, hash,
//                   byte_size, status }, … ]
//
//   GET  /api/v1/packages/:id/:version
//        200  : raw .nexor bytes
//        404  : { "error": "…" }
//
//   GET  /api/v1/admin/packages?status=pending|live|rolled_back  [auth: bearer]
//   POST /api/v1/admin/packages/:id/:version/deploy              [auth: bearer]
//   POST /api/v1/admin/packages/:id/:version/rollback            [auth: bearer]
//
// Auth model (Phase 9c): a single shared bearer token configured via Core's
// --admin-token CLI option.  When set, every "[auth: bearer]" route requires
//      Authorization: Bearer <token>
// otherwise the route returns 401 without touching the registry.  When the
// token is empty Core stays open (development default).
// =============================================================================
#ifndef NEXOR_CORE_PACKAGEAPI_H
#define NEXOR_CORE_PACKAGEAPI_H

#include <QDateTime>
#include <QString>

namespace nx {

class Router;
class PackageRegistry;

class PackageApi {
public:
    PackageApi(Router *router, PackageRegistry *registry);

    void setAdminToken(const QString &t) { m_adminToken = t; }

    void registerRoutes();

private:
    Router          *m_router;
    PackageRegistry *m_registry;
    QString          m_adminToken;
    QDateTime        m_startedAt;
};

} // namespace nx

#endif // NEXOR_CORE_PACKAGEAPI_H
