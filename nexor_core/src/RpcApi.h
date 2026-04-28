// =============================================================================
// RpcApi — registers the /api/v1/rpc/:package/:sub endpoint against a Router.
//
//   POST /api/v1/rpc/:package/:sub                         [auth: bearer]
//        Body: { "args": [ ... ] }   - JSON array of positional arguments.
//        200 : { "result": <json>, "kind": "<value-kind>" }
//        404 : { "error": "no live version" | "sub not found" }
//        400 : { "error": "..." }    - parse/auth/runs-on rule violation
//
// Lookup rules:
//   - Identifies the LIVE version of the named package (only one live row
//     exists by construction; PackageRegistry::deploy enforces this).
//   - Loads its activities into a fresh Interpreter (HostRole::Server).
//   - Calls the named sub.  Refuses [ClientOnly] bodies.
//
// Errors and Print output flow back as JSON (errors -> 500 with the message;
// Print is captured but currently dropped — Phase 14b' surfaces it).
// =============================================================================
#ifndef NEXOR_CORE_RPCAPI_H
#define NEXOR_CORE_RPCAPI_H

#include <QString>

namespace nx {

class Router;
class PackageRegistry;

class RpcApi {
public:
    RpcApi(Router *router, PackageRegistry *registry);

    void setAdminToken(const QString &t) { m_adminToken = t; }
    void registerRoutes();

private:
    Router          *m_router;
    PackageRegistry *m_registry;
    QString          m_adminToken;
};

} // namespace nx

#endif // NEXOR_CORE_RPCAPI_H
