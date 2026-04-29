// Quick verifier: load each .aba listed on the command line and dump its
// Forms list.  Used to confirm Activity::load handles both <Form file=".."/>
// and legacy <Form>name</Form> shapes.
#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>

#include "project/Activity.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    int failed = 0;
    for (int i = 1; i < argc; ++i) {
        Activity a;
        a.setFilePath(argv[i]);
        QString name = QFileInfo(argv[i]).fileName();
        if (!a.load()) {
            out << "FAIL load " << name << Qt::endl;
            ++failed;
            continue;
        }
        out << name << "  id=" << a.meta().id
            << "  forms=" << a.forms().size() << Qt::endl;
        for (const QString &f : a.forms())
            out << "    - " << f << Qt::endl;
        if (a.forms().isEmpty()) ++failed;
    }
    return failed;
}
