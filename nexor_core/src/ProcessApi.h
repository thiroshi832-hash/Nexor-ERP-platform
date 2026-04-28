// =============================================================================
// ProcessApi — persistent process instances + the start/resume/list/get
// endpoints from the architecture's wire-protocol section (page 8 +
// the "Persistent state machines" row on page 7).
//
//   POST /api/v1/processes/:package/:process/start    [auth: bearer]
//        body = {vars: {...}}        - initial Vars bag (optional)
//        201  = {instance, status, awaiting_form, awaiting_step, vars, ...}
//
//   POST /api/v1/processes/:instance/resume           [auth: bearer]
//        body = {vars: {...}}        - vars overrides on resume
//        200  = {instance, status, ...}
//
//   GET  /api/v1/processes                            [auth: bearer]
//        ?status=awaiting_human|completed|failed       optional filter
//        200  = [{instance, ...}]
//
//   GET  /api/v1/processes/:instance                  [auth: bearer]
//        200 / 404
//
// SQLite schema:
//
//   CREATE TABLE process_instances (
//     instance       INTEGER PRIMARY KEY AUTOINCREMENT,
//     package_id     TEXT NOT NULL,
//     process_id     TEXT NOT NULL,
//     status         TEXT NOT NULL,        -- pending|awaiting_human|completed|failed
//     awaiting_step  TEXT,
//     awaiting_form  TEXT,
//     vars_json      TEXT NOT NULL,        -- JSON object; survives restarts
//     last_error     TEXT,
//     started_at     TEXT NOT NULL,        -- ISO8601 UTC
//     updated_at     TEXT NOT NULL
//   );
//
// Phase 16b will WebSocket-push the awaiting_human transitions to Flux.
// =============================================================================
#ifndef NEXOR_CORE_PROCESSAPI_H
#define NEXOR_CORE_PROCESSAPI_H

#include <QString>

class QSqlDatabase;

namespace nx {

class Router;
class PackageRegistry;

class ProcessApi {
public:
    ProcessApi(Router *router, PackageRegistry *registry);
    ~ProcessApi();

    void setAdminToken(const QString &t) { m_adminToken = t; }

    // Open the persistence DB under the same data root as the registry.
    bool open(const QString &dataRoot, QString *error = nullptr);

    void registerRoutes();

private:
    Router          *m_router;
    PackageRegistry *m_registry;
    QString          m_adminToken;
    QString          m_dbName;     // unique connection
};

} // namespace nx

#endif // NEXOR_CORE_PROCESSAPI_H
