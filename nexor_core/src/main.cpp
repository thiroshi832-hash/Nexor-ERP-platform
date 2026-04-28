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

    nx::Router router;
    nx::PackageApi api(&router, &registry);
    api.setAdminToken(adminToken);
    api.registerRoutes();

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
    qInfo().noquote() << QString("  signing-key : %1").arg(
        signingKey.isEmpty() ? "<permissive>" : "<set>");
    qInfo().noquote() << QString("  admin-token : %1").arg(
        adminToken.isEmpty() ? "<open>"       : "<set>");
    return app.exec();
}
