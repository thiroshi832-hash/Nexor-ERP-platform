// =============================================================================
// Nexor Core — backend server entry point.
//
//     NexorCore [--port 7421] [--data <path>]
//                [--signing-key <key>] [--admin-token <token>]
//                [--headless]
//
// Without --headless Core launches its Qt Widgets shell - a small window
// with environment-variable settings, server status, and a live log.
// With --headless it runs as a background process (no window) and obeys
// only the CLI flags + persisted QSettings.
//
// Settings persist to QSettings under "Server/*" so the GUI remembers
// across sessions; CLI flags override.
// =============================================================================
#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>

#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("NexorCore");
    QCoreApplication::setApplicationVersion("0.1.0");
    QCoreApplication::setOrganizationName("Nexor");
    QApplication::setWindowIcon(QIcon(":/icons/app.png"));

    QCommandLineParser parser;
    parser.setApplicationDescription("Nexor backend server (GUI + HTTP).");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption       ({"p","port"},
        "Listening port override.",            "port");
    QCommandLineOption dataOption       ({"d","data"},
        "Data root override.",                  "path");
    QCommandLineOption signingKeyOption ("signing-key",
        "HMAC-SHA256 signing key override.",    "key");
    QCommandLineOption adminTokenOption ("admin-token",
        "Admin bearer token override.",         "token");
    QCommandLineOption headlessOption   ("headless",
        "Run without the GUI shell (background daemon).");
    parser.addOption(portOption);
    parser.addOption(dataOption);
    parser.addOption(signingKeyOption);
    parser.addOption(adminTokenOption);
    parser.addOption(headlessOption);
    parser.process(app);

    nx::MainWindow win;
    win.applyCliOverrides(
        parser.value(portOption),
        parser.value(dataOption),
        parser.value(signingKeyOption),
        parser.value(adminTokenOption));
    win.startServer();

    if (!parser.isSet(headlessOption))
        win.show();
    return app.exec();
}
