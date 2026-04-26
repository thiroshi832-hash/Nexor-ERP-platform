// =============================================================================
// Nexor Core — backend server entry point.
// =============================================================================
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "CoreServer.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("NexorCore");
    QCoreApplication::setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Nexor backend server");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(
        QStringList() << "p" << "port",
        "Listening port (default: 7421).", "port", "7421");
    parser.addOption(portOption);
    parser.process(app);

    quint16 port = static_cast<quint16>(parser.value(portOption).toUInt());

    CoreServer server;
    if (!server.start(port)) {
        qCritical() << "Failed to start NexorCore on port" << port;
        return 1;
    }

    qInfo().noquote() << QString("NexorCore listening on port %1").arg(port);
    return app.exec();
}
