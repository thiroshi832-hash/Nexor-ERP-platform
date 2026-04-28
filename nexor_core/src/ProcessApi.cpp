#include "ProcessApi.h"
#include "Http.h"
#include "PackageRegistry.h"
#include "ValueJson.h"
#include "../../nexor_studio/src/build/Package.h"
#include "../../nexor_studio/src/build/PackageReader.h"
#include "../../nexor_studio/src/project/Process.h"
#include "../../nexor_studio/src/project/BpmnIo.h"
#include "../../nexor_studio/src/runtime/ProcessEngine.h"
#include "../../nexor_studio/src/language/Value.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QUuid>
#include <QDateTime>
#include <QUrlQuery>

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

QJsonObject varsToJson(const ProcessEngine::Vars &v) {
    QJsonObject o;
    for (auto it = v.constBegin(); it != v.constEnd(); ++it)
        o.insert(it.key(), valueToJson(it.value()));
    return o;
}

ProcessEngine::Vars varsFromJson(const QJsonObject &o) {
    ProcessEngine::Vars v;
    for (auto it = o.constBegin(); it != o.constEnd(); ++it)
        v.insert(it.key(), jsonToValue(it.value()));
    return v;
}

// Find the live version of `pkgId` and return the parsed Package.
bool loadLivePackage(PackageRegistry *reg, const QString &pkgId,
                     Package &out, QString *error) {
    for (const auto &r : reg->list("live")) {
        if (r.id != pkgId) continue;
        QFile f(r.filePath);
        if (!f.open(QIODevice::ReadOnly)) {
            if (error) *error = "cannot read live package on disk";
            return false;
        }
        auto rd = PackageReader::fromBytes(f.readAll(), false);
        if (rd.status != PackageReader::Status::Ok) {
            if (error) *error = "stored package failed to parse";
            return false;
        }
        out = rd.package;
        return true;
    }
    if (error) *error = QString("no live version of '%1'").arg(pkgId);
    return false;
}

// Locate a process by id inside the package and parse it via BpmnIo if it
// looks like BPMN, else fall back to the legacy .prc.
bool extractProcess(const Package &pkg, const QString &processId,
                    Process &out, QString *error) {
    for (const PackageEntry &pe : pkg.processes) {
        if (pe.id != processId) continue;
        QByteArray bytes = pe.content.toUtf8();
        if (BpmnIo::sniff(bytes)) {
            QString berr;
            if (!BpmnIo::readBpmn(bytes, out, &berr)) {
                if (error) *error = "BPMN parse: " + berr;
                return false;
            }
            return true;
        }
        // Legacy .prc - write to temp + load via Process.
        QString tmpDir = QDir::tempPath() + "/nexor_proc_" +
                         QUuid::createUuid().toString(QUuid::WithoutBraces);
        QDir().mkpath(tmpDir);
        QString tmpPath = tmpDir + "/" + processId + ".prc";
        QFile f(tmpPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (error) *error = "tmp write failed";
            return false;
        }
        f.write(bytes);
        f.close();
        out.setFilePath(tmpPath);
        if (!out.load()) {
            if (error) *error = "legacy .prc parse failed";
            return false;
        }
        return true;
    }
    if (error) *error = QString("no process '%1' in package").arg(processId);
    return false;
}

QJsonObject instanceRow(QSqlQuery &q) {
    QJsonObject o;
    o.insert("instance",       static_cast<double>(q.value(0).toLongLong()));
    o.insert("package_id",     q.value(1).toString());
    o.insert("process_id",     q.value(2).toString());
    o.insert("status",         q.value(3).toString());
    o.insert("awaiting_step",  q.value(4).toString());
    o.insert("awaiting_form",  q.value(5).toString());
    QJsonDocument vdoc = QJsonDocument::fromJson(q.value(6).toString().toUtf8());
    o.insert("vars",           vdoc.object());
    o.insert("last_error",     q.value(7).toString());
    o.insert("started_at",     q.value(8).toString());
    o.insert("updated_at",     q.value(9).toString());
    return o;
}

const char *kInstColumns =
    " instance, package_id, process_id, status, awaiting_step, awaiting_form,"
    " vars_json, last_error, started_at, updated_at ";

} // namespace

ProcessApi::ProcessApi(Router *router, PackageRegistry *registry)
    : m_router(router), m_registry(registry),
      m_dbName(QString("nexor_proc_%1")
                   .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))) {}

ProcessApi::~ProcessApi() {
    auto db = QSqlDatabase::database(m_dbName);
    if (db.isOpen()) db.close();
    QSqlDatabase::removeDatabase(m_dbName);
}

bool ProcessApi::open(const QString &dataRoot, QString *error) {
    QString dbPath = QDir(dataRoot).absoluteFilePath("core_processes.db");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_dbName);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        if (error) *error = db.lastError().text();
        return false;
    }
    QSqlQuery q(db);
    if (!q.exec(
        "CREATE TABLE IF NOT EXISTS process_instances ("
        "  instance       INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  package_id     TEXT NOT NULL,"
        "  process_id     TEXT NOT NULL,"
        "  status         TEXT NOT NULL,"
        "  awaiting_step  TEXT,"
        "  awaiting_form  TEXT,"
        "  vars_json      TEXT NOT NULL,"
        "  last_error     TEXT,"
        "  started_at     TEXT NOT NULL,"
        "  updated_at     TEXT NOT NULL"
        ")")) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

void ProcessApi::registerRoutes() {
    auto db = QSqlDatabase::database(m_dbName);

    // POST /api/v1/processes/:package/:process/start
    m_router->route("POST", "/api/v1/processes/:package/:process/start",
        [reg = m_registry, dbName = m_dbName, tok = m_adminToken]
        (const HttpRequest &req, HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QString pkgId  = req.pathParams.value("package");
            QString procId = req.pathParams.value("process");

            Package pkg;
            QString err;
            if (!loadLivePackage(reg, pkgId, pkg, &err)) {
                res.setStatus(404, "Not Found");
                res.setJson(jsonError(err));
                return;
            }
            Process proc;
            if (!extractProcess(pkg, procId, proc, &err)) {
                res.setStatus(404, "Not Found");
                res.setJson(jsonError(err));
                return;
            }

            // Initial vars.
            QJsonObject body = QJsonDocument::fromJson(req.body).object();
            ProcessEngine::Vars vars =
                varsFromJson(body.value("vars").toObject());

            QStringList captured;
            auto outFn = [&captured](const QString &l){ captured.append(l); };
            QString rtErr;
            auto errFn = [&rtErr](const QString &e){ if (rtErr.isEmpty()) rtErr = e; };

            ProcessEngine::StepResult sr =
                ProcessEngine::runHeadless(proc, QString(), vars,
                                           outFn, errFn, nullptr);

            QString status =
                sr.state == ProcessEngine::RunState::Completed     ? "completed" :
                sr.state == ProcessEngine::RunState::AwaitingHuman ? "awaiting_human"
                                                                   : "failed";
            QByteArray varsJson = QJsonDocument(varsToJson(sr.vars))
                                      .toJson(QJsonDocument::Compact);
            QDateTime now = QDateTime::currentDateTimeUtc();

            QSqlDatabase db2 = QSqlDatabase::database(dbName);
            QSqlQuery ins(db2);
            ins.prepare(
                "INSERT INTO process_instances"
                " (package_id, process_id, status, awaiting_step, awaiting_form,"
                "  vars_json, last_error, started_at, updated_at)"
                " VALUES (:pkg, :proc, :st, :as, :af, :vars, :err, :s, :u)");
            ins.bindValue(":pkg",  pkgId);
            ins.bindValue(":proc", procId);
            ins.bindValue(":st",   status);
            ins.bindValue(":as",   sr.awaitingStepId);
            ins.bindValue(":af",   sr.awaitingFormId);
            ins.bindValue(":vars", QString::fromUtf8(varsJson));
            ins.bindValue(":err",  sr.lastError);
            ins.bindValue(":s",    now.toString(Qt::ISODate));
            ins.bindValue(":u",    now.toString(Qt::ISODate));
            if (!ins.exec()) {
                res.setStatus(500, "Internal Server Error");
                res.setJson(jsonError("DB insert failed: " + ins.lastError().text()));
                return;
            }
            qint64 instance = ins.lastInsertId().toLongLong();

            QJsonObject o;
            o.insert("instance",       static_cast<double>(instance));
            o.insert("package_id",     pkgId);
            o.insert("process_id",     procId);
            o.insert("status",         status);
            o.insert("awaiting_step",  sr.awaitingStepId);
            o.insert("awaiting_form",  sr.awaitingFormId);
            o.insert("vars",           QJsonDocument::fromJson(varsJson).object());
            o.insert("last_error",     sr.lastError);
            QJsonArray oa;
            for (const auto &l : captured) oa.append(l);
            o.insert("output", oa);
            res.setStatus(201, "Created");
            res.setJson(QJsonDocument(o).toJson(QJsonDocument::Compact));
        });

    // POST /api/v1/processes/:instance/resume
    m_router->route("POST", "/api/v1/processes/:instance/resume",
        [reg = m_registry, dbName = m_dbName, tok = m_adminToken]
        (const HttpRequest &req, HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            qint64 instance = req.pathParams.value("instance").toLongLong();

            QSqlDatabase db2 = QSqlDatabase::database(dbName);
            QSqlQuery sel(db2);
            sel.prepare(QString("SELECT %1 FROM process_instances"
                                " WHERE instance = :i").arg(kInstColumns));
            sel.bindValue(":i", instance);
            if (!sel.exec() || !sel.next()) {
                res.setStatus(404, "Not Found");
                res.setJson(jsonError(QString("instance %1 not found").arg(instance)));
                return;
            }
            QString pkgId  = sel.value(1).toString();
            QString procId = sel.value(2).toString();
            QString status = sel.value(3).toString();
            QString awStep = sel.value(4).toString();
            if (status != "awaiting_human") {
                res.setStatus(400, "Bad Request");
                res.setJson(jsonError(
                    QString("instance %1 is %2, not awaiting_human")
                        .arg(instance).arg(status)));
                return;
            }
            QJsonObject vobj = QJsonDocument::fromJson(
                sel.value(6).toString().toUtf8()).object();

            // Merge incoming overrides on top.
            QJsonObject body = QJsonDocument::fromJson(req.body).object();
            QJsonObject overrides = body.value("vars").toObject();
            for (auto it = overrides.constBegin(); it != overrides.constEnd(); ++it)
                vobj.insert(it.key(), it.value());

            Package pkg;
            QString err;
            if (!loadLivePackage(reg, pkgId, pkg, &err)) {
                res.setStatus(404, "Not Found");
                res.setJson(jsonError(err));
                return;
            }
            Process proc;
            if (!extractProcess(pkg, procId, proc, &err)) {
                res.setStatus(404, "Not Found");
                res.setJson(jsonError(err));
                return;
            }

            // Resume from the step AFTER the awaiting one - the body of the
            // HumanTask runs server-side here, then we jump to its nextId.
            int idx = proc.indexOfStep(awStep);
            if (idx < 0) {
                res.setStatus(500, "Internal Server Error");
                res.setJson(jsonError("awaiting_step disappeared on resume"));
                return;
            }
            const StepSpec &awSt = proc.steps().at(idx);
            QString nextStep = awSt.nextId;

            // Run the body as part of the HumanTask resume.  We seed a
            // small headless run starting at the nextId so the engine
            // continues the flow naturally.
            QStringList captured;
            QString rtErr;
            ProcessEngine::Vars vars = varsFromJson(vobj);
            // First run the HumanTask body itself if it has one.  Wrap it
            // as a one-step pseudo-process by reusing runHeadless against
            // the real process starting at the awaiting step's nextId
            // (the body executes implicitly when the engine jumps onto
            // a Server step).  If the awaiting step had a code body, we
            // don't have a clean way to invoke just that body here -
            // future Phase 16c will surface a "post-form" body hook.

            ProcessEngine::StepResult sr =
                ProcessEngine::runHeadless(proc, nextStep, vars,
                    [&captured](const QString &l){ captured.append(l); },
                    [&rtErr](const QString &e){ if (rtErr.isEmpty()) rtErr = e; },
                    nullptr);

            QString newStatus =
                sr.state == ProcessEngine::RunState::Completed     ? "completed" :
                sr.state == ProcessEngine::RunState::AwaitingHuman ? "awaiting_human"
                                                                   : "failed";
            QByteArray newVarsJson = QJsonDocument(varsToJson(sr.vars))
                                         .toJson(QJsonDocument::Compact);
            QDateTime now = QDateTime::currentDateTimeUtc();
            QSqlQuery up(db2);
            up.prepare(
                "UPDATE process_instances SET"
                "  status=:st, awaiting_step=:as, awaiting_form=:af,"
                "  vars_json=:vars, last_error=:err, updated_at=:u"
                "  WHERE instance=:i");
            up.bindValue(":st",   newStatus);
            up.bindValue(":as",   sr.awaitingStepId);
            up.bindValue(":af",   sr.awaitingFormId);
            up.bindValue(":vars", QString::fromUtf8(newVarsJson));
            up.bindValue(":err",  sr.lastError);
            up.bindValue(":u",    now.toString(Qt::ISODate));
            up.bindValue(":i",    instance);
            if (!up.exec()) {
                res.setStatus(500, "Internal Server Error");
                res.setJson(jsonError("DB update failed: " + up.lastError().text()));
                return;
            }

            QJsonObject o;
            o.insert("instance",       static_cast<double>(instance));
            o.insert("package_id",     pkgId);
            o.insert("process_id",     procId);
            o.insert("status",         newStatus);
            o.insert("awaiting_step",  sr.awaitingStepId);
            o.insert("awaiting_form",  sr.awaitingFormId);
            o.insert("vars",           QJsonDocument::fromJson(newVarsJson).object());
            o.insert("last_error",     sr.lastError);
            QJsonArray oa;
            for (const auto &l : captured) oa.append(l);
            o.insert("output", oa);
            res.setJson(QJsonDocument(o).toJson(QJsonDocument::Compact));
        });

    // GET /api/v1/processes
    m_router->route("GET", "/api/v1/processes",
        [dbName = m_dbName, tok = m_adminToken]
        (const HttpRequest &req, HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            QSqlDatabase db2 = QSqlDatabase::database(dbName);
            QUrlQuery qq(req.query);
            QString filter = qq.queryItemValue("status");
            QString sql = QString("SELECT %1 FROM process_instances").arg(kInstColumns);
            if (!filter.isEmpty()) sql += " WHERE status = :st";
            sql += " ORDER BY updated_at DESC, instance DESC";
            QSqlQuery q(db2);
            q.prepare(sql);
            if (!filter.isEmpty()) q.bindValue(":st", filter);
            q.exec();
            QJsonArray arr;
            while (q.next()) arr.append(instanceRow(q));
            res.setJson(QJsonDocument(arr).toJson(QJsonDocument::Compact));
        });

    // GET /api/v1/processes/:instance
    m_router->route("GET", "/api/v1/processes/:instance",
        [dbName = m_dbName, tok = m_adminToken]
        (const HttpRequest &req, HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;
            qint64 i = req.pathParams.value("instance").toLongLong();
            QSqlDatabase db2 = QSqlDatabase::database(dbName);
            QSqlQuery q(db2);
            q.prepare(QString("SELECT %1 FROM process_instances"
                              " WHERE instance = :i").arg(kInstColumns));
            q.bindValue(":i", i);
            if (!q.exec() || !q.next()) {
                res.setStatus(404, "Not Found");
                res.setJson(jsonError(QString("instance %1 not found").arg(i)));
                return;
            }
            res.setJson(QJsonDocument(instanceRow(q)).toJson(QJsonDocument::Compact));
        });
}

} // namespace nx
