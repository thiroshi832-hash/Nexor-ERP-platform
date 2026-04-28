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
    parser.addOption(portOption);
    parser.addOption(dataOption);
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

    nx::Router router;
    nx::PackageApi api(&router, &registry);
    api.registerRoutes();

    nx::HttpServer server(&router);
    if (!server.start(port)) {
        qCritical().noquote() << "NexorCore: cannot bind port" << port;
        return 1;
    }

    qInfo().noquote() << QString("NexorCore listening on http://0.0.0.0:%1").arg(port);
    qInfo().noquote() << "  GET  /api/v1/health";
    qInfo().noquote() << "  GET  /api/v1/packages";
    qInfo().noquote() << "  POST /api/v1/packages       (raw .nexor bytes)";
    qInfo().noquote() << "  GET  /api/v1/packages/:id/:version";
    return app.exec();
}
