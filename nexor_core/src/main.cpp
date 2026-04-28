// =============================================================================
// Nexor Core — backend server entry point.
//
//     NexorCore [--port 7421] [--data <path>]
//
// Without --data the server keeps state under the user's per-profile data
// directory (Qt's GenericDataLocation), which works on every host without
// extra configuration but can be overridden for tests / multi-tenant runs.
// =============================================================================
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

#include "Http.h"
#include "PackageRegistry.h"
#include "PackageApi.h"
#include "RpcApi.h"
#include "EntityApi.h"
#include "CoreEntityStore.h"
#include "ProcessApi.h"
#include "../../nexor_studio/src/build/PackageReader.h"
#include <QFile>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("NexorCore");
    QCoreApplication::setApplicationVersion("0.1.0");
    QCoreApplication::setOrganizationName("Nexor");

    QCommandLineParser parser;
    parser.setApplicationDescription("Nexor backend server");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption({"p", "port"},
        "Listening port (default: 7421).", "port", "7421");
    QCommandLineOption dataOption({"d", "data"},
        "Data root (default: %APPDATA%/Nexor/Core).", "path");
    QCommandLineOption signingKeyOption("signing-key",
        "Require this HMAC-SHA256 key on every package upload. "
        "Empty = permissive (default).", "key");
    QCommandLineOption adminTokenOption("admin-token",
        "Require this bearer token on POST + /admin/* requests. "
        "Empty = open (default).", "token");
    parser.addOption(portOption);
    parser.addOption(dataOption);
    parser.addOption(signingKeyOption);
    parser.addOption(adminTokenOption);
    parser.process(app);

    quint16 port = static_cast<quint16>(parser.value(portOption).toUInt());
    QString dataRoot = parser.isSet(dataOption)
        ? parser.value(dataOption)
        : QDir(QStandardPaths::writableLocation(
              QStandardPaths::GenericDataLocation)).absoluteFilePath("Nexor/Core");

    nx::PackageRegistry registry(dataRoot);
    QString err;
    if (!registry.open(&err)) {
        qCritical().noquote() << "NexorCore: registry open failed —" << err;
        return 2;
    }
    QString signingKey = parser.value(signingKeyOption);
    QString adminToken = parser.value(adminTokenOption);
    if (!signingKey.isEmpty()) registry.setSigningKey(signingKey.toUtf8());

    // Entity store - opens its own SQLite (core_entities.db) and gains a
    // schema for every Sheet in every published package as it lands.
    nx::CoreEntityStore entities(dataRoot);
    if (!entities.open(&err)) {
        qCritical().noquote() << "NexorCore: entity store open failed —" << err;
        return 3;
    }
    registry.setPackageListener([&entities](const QByteArray &bytes){
        auto rd = nx::PackageReader::fromBytes(bytes, /*verifyHash*/false);
        if (rd.status == nx::PackageReader::Status::Ok)
            entities.registerSheetsFromPackage(rd.package);
    });
    // Also walk what's already on disk so a fresh boot picks up sheets
    // from packages that were stored before this code shipped.
    for (const auto &row : registry.list()) {
        QFile f(row.filePath);
        if (!f.open(QIODevice::ReadOnly)) continue;
        auto rd = nx::PackageReader::fromBytes(f.readAll(), /*verifyHash*/false);
        if (rd.status == nx::PackageReader::Status::Ok)
            entities.registerSheetsFromPackage(rd.package);
    }

    nx::Router router;
    nx::PackageApi api(&router, &registry);
    api.setAdminToken(adminToken);
    api.registerRoutes();

    nx::RpcApi rpc(&router, &registry);
    rpc.setAdminToken(adminToken);
    rpc.registerRoutes();

    nx::EntityApi entityApi(&router, &entities);
    entityApi.setAdminToken(adminToken);
    entityApi.registerRoutes();

    nx::ProcessApi procApi(&router, &registry);
    if (!procApi.open(dataRoot, &err)) {
        qCritical().noquote() << "NexorCore: process store open failed —" << err;
        return 4;
    }
    procApi.setAdminToken(adminToken);
    procApi.registerRoutes();

    nx::HttpServer server(&router);
    if (!server.start(port)) {
        qCritical().noquote() << "NexorCore: cannot bind port" << port;
        return 1;
    }

    qInfo().noquote() << QString("NexorCore listening on http://0.0.0.0:%1").arg(port);
    qInfo().noquote() << "  GET  /api/v1/health";
    qInfo().noquote() << "  GET  /api/v1/packages";
    qInfo().noquote() << "  POST /api/v1/packages                              (raw .nexor)";
    qInfo().noquote() << "  GET  /api/v1/packages/:id/:version";
    qInfo().noquote() << "  GET  /api/v1/admin/packages[?status=…]";
    qInfo().noquote() << "  POST /api/v1/admin/packages/:id/:version/deploy";
    qInfo().noquote() << "  POST /api/v1/admin/packages/:id/:version/rollback";
    qInfo().noquote() << "  POST /api/v1/rpc/:package/:sub                      (ServerOnly subs)";
    qInfo().noquote() << "  GET  /api/v1/entities/:sheet[/:id]";
    qInfo().noquote() << "  POST /api/v1/entities/:sheet";
    qInfo().noquote() << "  PATCH/DELETE /api/v1/entities/:sheet/:id";
    qInfo().noquote() << "  POST /api/v1/processes/:package/:process/start";
    qInfo().noquote() << "  POST /api/v1/processes/:instance/resume";
    qInfo().noquote() << "  GET  /api/v1/processes[?status=...]";
    qInfo().noquote() << "  GET  /api/v1/processes/:instance";
    qInfo().noquote() << QString("  signing-key : %1").arg(
        signingKey.isEmpty() ? "<permissive>" : "<set>");
    qInfo().noquote() << QString("  admin-token : %1").arg(
        adminToken.isEmpty() ? "<open>"       : "<set>");
    return app.exec();
}
