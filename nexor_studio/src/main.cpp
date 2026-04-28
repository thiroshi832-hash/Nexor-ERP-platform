// =============================================================================
// Nexor Studio — IDE entry point.
// =============================================================================
#include <QApplication>
#include <QStyleFactory>
#include <QIcon>
#include "ide/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("NexorStudio");
    QApplication::setOrganizationName("Nexor");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication::setWindowIcon(QIcon(":/icons/app.png"));

    MainWindow w;
    w.show();
    return app.exec();
}
