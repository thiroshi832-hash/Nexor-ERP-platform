// =============================================================================
// Nexor Command — admin app entry point.
// =============================================================================
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("NexorCommand");
    QCoreApplication::setApplicationVersion("0.1.0");
    QCoreApplication::setOrganizationName("Nexor");

    nx::MainWindow win;
    win.show();
    return app.exec();
}
