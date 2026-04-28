// =============================================================================
// PackageApi — registers the /api/v1/packages and /api/v1/health routes
// against a Router, backed by a PackageRegistry.
//
// Endpoints (Phase 9 — no auth yet):
//
//   GET  /api/v1/health
//        Body : { "service": "NexorCore", "version": "0.1.0", "uptime": <s> }
//
//   POST /api/v1/packages
//        Body : raw .nexor bytes  (Content-Type: application/x-nexor-package
//                                  or application/octet-stream)
//        200  : { "id":"…", "version":"…", "hash":"…", "status":"pending" }
//        400  : { "error": "…" }
//
//   GET  /api/v1/packages
//        200  : [ { id, version, title, built_at, received_at, hash,
//                   byte_size, status }, … ]
//
//   GET  /api/v1/packages/:id/:version
//        200  : raw .nexor bytes
//        404  : { "error": "…" }
//
// Phase 9b will gate the POST behind a signed token; Phase 9c will route
// per-tenant.  Both layers slot in on top of this without changing the wire
// shape.
// =============================================================================
#ifndef NEXOR_CORE_PACKAGEAPI_H
#define NEXOR_CORE_PACKAGEAPI_H

#include <QDateTime>

namespace nx {

class Router;
class PackageRegistry;

class PackageApi {
public:
    PackageApi(Router *router, PackageRegistry *registry);

    void registerRoutes();

private:
    Router          *m_router;
    PackageRegistry *m_registry;
    QDateTime        m_startedAt;
};

} // namespace nx

#endif // NEXOR_CORE_PACKAGEAPI_H
