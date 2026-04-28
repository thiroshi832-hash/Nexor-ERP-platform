// =============================================================================
// PackageCache — Flux's local package store.
//
//   <root>/                     (default: %APPDATA%/Nexor/Flux)
//     packages/<id>/<ver>.nexor       raw bytes as fetched from Core
//     extracted/<id>/<ver>/           Project-shaped layout produced
//                                     from the .nexor; FormRunner et al.
//                                     consume this directly.
//
// Extraction: we materialise the embedded artifacts back onto disk under
// the canonical project structure so the existing Studio runtime (Project
// loader, NexorRuntime, FormRunner, ProcessEngine) can work unchanged.
//
// The cache is a flat scan: list() reads the packages/ tree and pairs
// each version with its extracted root.  No SQLite needed yet.
// =============================================================================
#ifndef NEXOR_FLUX_PACKAGECACHE_H
#define NEXOR_FLUX_PACKAGECACHE_H

#include <QString>
#include <QVector>

#include "../../nexor_studio/src/build/Package.h"

class Project;

namespace nx {

struct InstalledRow {
    QString id;
    QString version;
    QString title;
    QString packagePath;          // packages/<id>/<ver>.nexor
    QString projectRoot;          // extracted/<id>/<ver>/
    QString projectFile;          // extracted/<id>/<ver>/<id>.pro
    qint64  byteSize  { 0 };
};

class PackageCache {
public:
    explicit PackageCache(const QString &root);

    QString root()         const { return m_root; }
    QString packagesDir()  const { return m_root + "/packages"; }
    QString extractedDir() const { return m_root + "/extracted"; }

    // Verifies/creates the directories, returns true on success.
    bool prepare(QString *error = nullptr);

    // Reads the packages/ tree and returns one row per <id>/<ver>.nexor,
    // sniffing the manifest title from the file's <Title> element.
    QVector<InstalledRow> list() const;

    // Where Flux should download a fresh .nexor for (id, version).
    QString packagePathFor(const QString &id, const QString &version) const;

    // Take an already-downloaded .nexor and explode it into the canonical
    // Project layout (.pro, activities/, sheets/, processes/, …).  The
    // generated .pro file references each artifact by relative path.
    bool installFromBytes(const QByteArray &bytes,
                          InstalledRow &outRow,
                          QString *error = nullptr);

    // Removes a (id, version) pair from disk — both packages and extracted
    // copies.  Used for "Uninstall".  Returns false if nothing to remove.
    bool uninstall(const QString &id, const QString &version);

private:
    QString m_root;

    // Internal helpers -----------------------------------------------
    static QString sniffTitle(const QString &nexorPath);
    bool writeProjectScaffold(const Package &pkg, const QString &projectRoot,
                              QString &outProjectFile, QString *error) const;
};

} // namespace nx

#endif // NEXOR_FLUX_PACKAGECACHE_H
