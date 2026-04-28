// =============================================================================
// Nexor Flux — desktop end-user runtime entry point.
// =============================================================================
#include <QApplication>
#include "FluxWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("NexorFlux");
    QCoreApplication::setApplicationVersion("0.1.0");
    QCoreApplication::setOrganizationName("Nexor");

    nx::FluxWindow w;
    w.show();
    return app.exec();
}
