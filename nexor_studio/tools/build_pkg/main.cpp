// Tiny harness:
//   build_pkg <project.pro> <version>
// Loads the project, runs PackageBuilder::buildAndWrite, then re-reads
// the package via PackageReader to confirm the hash round-trips.
// Exits 0 on success, prints the output path and the manifest hash.
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include <QFile>

#include "project/Project.h"
#include "build/PackageBuilder.h"
#include "build/PackageReader.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    if (argc < 3) {
        qInfo() << "usage: build_pkg <project.pro> <version>";
        return 1;
    }
    Project p;
    p.setFilePath(argv[1]);
    if (!p.load()) {
        qCritical() << "cannot load project" << argv[1];
        return 2;
    }
    qInfo().noquote() << QString("Loaded %1 (%2 activities, %3 sheets, %4 processes)")
                          .arg(p.meta().id)
                          .arg(p.atomicActivities().size())
                          .arg(p.sheets().size())
                          .arg(p.processActivities().size());

    auto res = nx::PackageBuilder::buildAndWrite(p, argv[2]);
    if (!res.ok) {
        qCritical().noquote() << "build failed:" << res.error;
        return 3;
    }
    qInfo().noquote() << "Wrote " << res.outputPath;
    qInfo().noquote() << "Hash: " << res.meta.hash;

    auto rd = nx::PackageReader::fromFile(res.outputPath);
    QString status =
        rd.status == nx::PackageReader::Status::Ok            ? "OK" :
        rd.status == nx::PackageReader::Status::HashMismatch  ? "HASH_MISMATCH" :
        rd.status == nx::PackageReader::Status::ParseError    ? "PARSE_ERROR" :
                                                                "OTHER";
    qInfo().noquote() << "Round-trip status:" << status;
    if (rd.status != nx::PackageReader::Status::Ok)
        qInfo().noquote() << "  message:" << rd.message;
    return rd.status == nx::PackageReader::Status::Ok ? 0 : 4;
}
