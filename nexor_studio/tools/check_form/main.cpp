// Smoke test for FormCanvas::loadForm.  Loads each .frm given on the
// command line and reports how many Widgets were actually instantiated
// on the design canvas, plus their type and name.
//
// If a sample .frm file shows N <Widget> elements on disk but this
// reports 0 items, then the loader is dropping them on the floor —
// not the disk content.
#include <QApplication>
#include <QFileInfo>
#include <QTextStream>
#include "designer/FormCanvas.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QTextStream out(stdout);
    int failed = 0;
    FormCanvas canvas;
    canvas.resize(800, 600);
    canvas.show();    // body needs to be visible for handles

    for (int i = 1; i < argc; ++i) {
        QString name = QFileInfo(argv[i]).fileName();
        if (!canvas.loadForm(argv[i])) {
            out << "FAIL load  " << name << Qt::endl;
            ++failed;
            continue;
        }
        const auto items = canvas.items();
        out << name << "  widgets=" << items.size() << Qt::endl;
        for (const auto &it : items) {
            out << "    - " << it.type << "  " << it.name
                << "  geom=" << (it.widget ? it.widget->geometry() : QRect()).x() << ","
                << (it.widget ? it.widget->geometry() : QRect()).y() << " "
                << (it.widget ? it.widget->geometry() : QRect()).width() << "x"
                << (it.widget ? it.widget->geometry() : QRect()).height()
                << Qt::endl;
        }
        if (items.isEmpty()) ++failed;
    }
    return failed;
}
