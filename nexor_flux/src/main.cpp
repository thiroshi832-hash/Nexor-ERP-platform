// =============================================================================
// Nexor Flux — client application entry point.
// =============================================================================
#include <QApplication>
#include "FluxWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("NexorFlux");
    QApplication::setApplicationVersion("0.1.0");

    FluxWindow w;
    w.show();
    return app.exec();
}
