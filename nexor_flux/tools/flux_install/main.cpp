// Tiny harness:
//   flux_install <root> <package.nexor>
// Installs the package into the Flux cache layout under <root>, then loads
// the resulting Project off disk to confirm the extraction is byte-clean.
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

#include "PackageCache.h"
#include "project/Project.h"
#include "project/Activity.h"
#include "project/Sheet.h"
#include "project/Process.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    if (argc < 3) {
        qInfo() << "usage: flux_install <root> <package.nexor>";
        return 1;
    }
    QString root  = argv[1];
    QString npath = argv[2];

    QFile f(npath);
    if (!f.open(QIODevice::ReadOnly)) {
        qCritical() << "cannot read" << npath;
        return 2;
    }
    QByteArray bytes = f.readAll();

    nx::PackageCache cache(root);
    QString err;
    if (!cache.prepare(&err)) { qCritical().noquote() << err; return 3; }

    nx::InstalledRow ins;
    if (!cache.installFromBytes(bytes, ins, &err)) {
        qCritical().noquote() << "install failed:" << err;
        return 4;
    }
    qInfo().noquote() << "Installed" << ins.id << "v" + ins.version;
    qInfo().noquote() << "  package:" << ins.packagePath;
    qInfo().noquote() << "  project:" << ins.projectFile;

    Project p;
    p.setFilePath(ins.projectFile);
    if (!p.load()) {
        qCritical() << "project load failed";
        return 5;
    }
    qInfo().noquote() << "Loaded:" << p.meta().title
                     << "  activities=" << p.atomicActivities().size()
                     << "  sheets=" << p.sheets().size()
                     << "  processes=" << p.processActivities().size();
    for (const auto &a : p.atomicActivities()) {
        qInfo().noquote() << "  Activity:" << a->meta().id
                         << "  forms=" << a->forms().size()
                         << "  file=" << a->filePath();
    }
    for (const auto &s : p.sheets()) {
        qInfo().noquote() << "  Sheet   :" << s->meta().id
                         << "  fields=" << s->fields().size();
    }
    return 0;
}
