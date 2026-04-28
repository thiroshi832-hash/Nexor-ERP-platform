// =============================================================================
// PackageRegistry — disk + SQLite store for published .nexor packages.
//
// Disk layout under <data root>:
//
//   data/
//     core.db                       SQLite metadata
//     packages/
//       <id>/
//         <version>.nexor           the actual artifact bytes
//
// SQLite schema:
//
//   CREATE TABLE package_versions (
//       id           TEXT NOT NULL,         -- project id (e.g. "Sales")
//       version      TEXT NOT NULL,         -- SemVer string
//       title        TEXT,
//       built_at     TEXT,                  -- ISO 8601 UTC from manifest
//       hash         TEXT,                  -- sha256 hex from manifest
//       hash_algo    TEXT,
//       byte_size    INTEGER,
//       received_at  TEXT,                  -- ISO 8601 UTC server-side
//       file_path    TEXT NOT NULL,
//       status       TEXT NOT NULL,         -- "pending" | "live" | "rolled_back"
//       PRIMARY KEY (id, version)
//   );
//
// Phase 9 keeps every upload in "pending" status — Command will flip rows
// to "live" when it lands.  Lookups by (id, version) are exact; the list
// endpoint returns all rows sorted by received_at desc.
// =============================================================================
#ifndef NEXOR_CORE_PACKAGEREGISTRY_H
#define NEXOR_CORE_PACKAGEREGISTRY_H

#include <QString>
#include <QVector>
#include <QByteArray>
#include <QDateTime>
#include <QSqlDatabase>

namespace nx {

struct RegistryRecord {
    QString   id;
    QString   version;
    QString   title;
    QDateTime builtAt;
    QString   hash;
    QString   hashAlgo;
    qint64    byteSize  { 0 };
    QDateTime receivedAt;
    QString   filePath;
    QString   status;          // "pending" | "live" | "rolled_back"
};

class PackageRegistry {
public:
    explicit PackageRegistry(const QString &dataRoot);
    ~PackageRegistry();

    bool open(QString *error = nullptr);
    bool isOpen() const;

    // Persists `bytes` under <root>/packages/<id>/<version>.nexor and inserts
    // (or updates) the metadata row.  The .nexor is parsed via PackageReader
    // (built into Core via the studio source tree) so we can pull its title /
    // hash / built-at out of the manifest.  Returns the resulting record.
    bool publish(const QByteArray &bytes,
                 RegistryRecord &outRecord,
                 QString *error = nullptr);

    QVector<RegistryRecord> list() const;
    bool                    find(const QString &id, const QString &version,
                                 RegistryRecord &out) const;
    QByteArray              read(const QString &id, const QString &version) const;

    QString rootDir() const { return m_root; }

private:
    bool ensureSchema(QString *error);

    QString       m_root;
    QSqlDatabase  m_db;
    QString       m_dbName;     // unique connection name
    bool          m_open { false };
};

} // namespace nx

#endif // NEXOR_CORE_PACKAGEREGISTRY_H
