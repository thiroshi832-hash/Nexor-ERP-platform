// Reads a .frm file, extracts its <Code> block, and compiles it through
// NexorRuntime.  Reports the compile error (or "OK") for each input.
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QXmlStreamReader>
#include "language/NexorRuntime.h"

static QString extractCode(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QXmlStreamReader r(&f);
    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement() && r.name() == "Code")
            return r.readElementText();
    }
    return {};
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    int failed = 0;
    for (int i = 1; i < argc; ++i) {
        QString name = QFileInfo(argv[i]).fileName();
        QString code = extractCode(argv[i]);
        if (code.isEmpty()) {
            out << name << "  no <Code> block" << Qt::endl;
            continue;
        }
        nx::NexorRuntime rt;
        if (!rt.compile(code, name)) {
            out << name << "  COMPILE FAIL: " << rt.lastError() << Qt::endl;
            ++failed;
        } else {
            out << name << "  OK" << Qt::endl;
        }
    }
    return failed;
}
