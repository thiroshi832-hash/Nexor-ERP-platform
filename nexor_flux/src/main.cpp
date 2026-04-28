// =============================================================================
// Nexor Flux — desktop end-user runtime entry point.
// =============================================================================
#include <QApplication>
#include <QIcon>
#include "FluxWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("NexorFlux");
    QCoreApplication::setApplicationVersion("0.1.0");
    QCoreApplication::setOrganizationName("Nexor");
    QApplication::setWindowIcon(QIcon(":/icons/app.png"));

    nx::FluxWindow w;
    w.show();
    return app.exec();
}
