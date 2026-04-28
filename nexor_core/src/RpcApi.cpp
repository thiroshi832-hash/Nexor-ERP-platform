#include "RpcApi.h"
#include "Http.h"
#include "PackageRegistry.h"
#include "ValueJson.h"
#include "../../nexor_studio/src/build/Package.h"
#include "../../nexor_studio/src/build/PackageReader.h"
#include "../../nexor_studio/src/language/NexorRuntime.h"
#include "../../nexor_studio/src/language/Interpreter.h"
#include "../../nexor_studio/src/language/Value.h"
#include "../../nexor_studio/src/language/EntityStore.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QXmlStreamReader>

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

// Concatenates every Activity's source body into a single compilation unit.
// Sub names are unique by Studio convention; on collision the last
// declaration wins (matches Interpreter::load behaviour).
QString collectActivitySource(const Package &pkg) {
    QString src;
    for (const PackageEntry &ae : pkg.activities) {
        QXmlStreamReader r(ae.content);
        while (!r.atEnd()) {
            r.readNext();
            if (r.isStartElement() && r.name() == "Code") {
                src += r.readElementText();
                src += "\n";
            }
        }
    }
    return src;
}

} // namespace

RpcApi::RpcApi(Router *router, PackageRegistry *registry)
    : m_router(router), m_registry(registry) {}

void RpcApi::registerRoutes() {
    m_router->route("POST", "/api/v1/rpc/:package/:sub",
        [reg = m_registry, tok = m_adminToken](const HttpRequest &req,
                                                HttpResponse &res) {
            if (!checkBearer(req, tok, res)) return;

            QString pkgId  = req.pathParams.value("package");
            QString subKey = req.pathParams.value("sub");

            // Find the live version.
            RegistryRecord chosen;
            bool foundLive = false;
            for (const auto &r : reg->list("live")) {
                if (r.id == pkgId) { chosen = r; foundLive = true; break; }
            }
            if (!foundLive) {
                res.setStatus(404, "Not Found");
                res.setJson(jsonError(
                    QString("No live version of '%1'.").arg(pkgId)));
                return;
            }
            QFile f(chosen.filePath);
            if (!f.open(QIODevice::ReadOnly)) {
                res.setStatus(500, "Internal Server Error");
                res.setJson(jsonError("Cannot read live package on disk."));
                return;
            }
            auto rd = PackageReader::fromBytes(f.readAll(), /*verifyHash*/false);
            if (rd.status != PackageReader::Status::Ok) {
                res.setStatus(500, "Internal Server Error");
                res.setJson(jsonError("Stored package failed to parse: "
                                       + rd.message));
                return;
            }

            // Parse args
            QJsonDocument body = QJsonDocument::fromJson(req.body);
            QVector<Value> args;
            if (body.isObject()) {
                QJsonArray arr = body.object().value("args").toArray();
                for (const auto &j : arr) args.append(jsonToValue(j));
            }

            // Compile + register sheets so the sub can use entity types.
            NexorRuntime rt;
            rt.interpreter()->setHostRole(Interpreter::HostRole::Server);

            // Pipe Print + errors into a captured buffer for the response.
            QStringList captured;
            rt.setOutput([&captured](const QString &line){
                captured.append(line);
            });
            QString rtErr;
            rt.setError([&rtErr](const QString &er){
                if (rtErr.isEmpty()) rtErr = er;
            });

            // Register the package's Sheets so entity references resolve.
            for (const PackageEntry &se : rd.package.sheets) {
                SheetSchema schema;
                schema.sheetId = se.id;
                // Crude parse — we only need name/type/key/required for
                // the runtime; identical to Studio's Sheet::load.
                QXmlStreamReader r(se.content);
                while (!r.atEnd()) {
                    r.readNext();
                    if (!r.isStartElement() || r.name() != "Field") continue;
                    SheetSchemaField f;
                    const auto a = r.attributes();
                    f.name        = a.value("name").toString();
                    f.type        = a.value("type").toString();
                    if (f.type.isEmpty()) f.type = "String";
                    f.isKey       = a.value("key").toString() == "true";
                    f.required    = a.value("required").toString() == "true";
                    f.defaultText = a.value("default").toString();
                    if (!f.name.isEmpty()) schema.fields.append(f);
                }
                rt.interpreter()->registerSheet(schema);
            }

            QString src = collectActivitySource(rd.package);
            if (!rt.compile(src, pkgId)) {
                res.setStatus(500, "Internal Server Error");
                res.setJson(jsonError("Compile error: " + rt.lastError()));
                return;
            }
            if (!rt.hasSub(subKey)) {
                res.setStatus(404, "Not Found");
                res.setJson(jsonError(
                    QString("Sub '%1' not found in '%2'.").arg(subKey, pkgId)));
                return;
            }
            Value out = rt.call(subKey, args);
            if (!rtErr.isEmpty()) {
                res.setStatus(500, "Internal Server Error");
                QJsonObject o;
                o.insert("error", rtErr);
                if (!captured.isEmpty()) {
                    QJsonArray a;
                    for (const auto &l : captured) a.append(l);
                    o.insert("output", a);
                }
                res.setJson(QJsonDocument(o).toJson(QJsonDocument::Compact));
                return;
            }

            QJsonObject ok;
            ok.insert("result", valueToJson(out));
            QJsonArray oa;
            for (const auto &l : captured) oa.append(l);
            ok.insert("output", oa);
            res.setJson(QJsonDocument(ok).toJson(QJsonDocument::Compact));
        });
}

} // namespace nx
