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
#include <functional>

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

// One row of the append-only audit log.  Every state-changing endpoint
// (publish / deploy / rollback / delete) records one of these so the admin
// has a complete who-did-what timeline.
struct AuditEvent {
    qint64    id { 0 };
    QString   eventType;       // "publish" | "deploy" | "rollback" | "delete-pending"
    QString   packageId;
    QString   packageVersion;
    QString   actor;           // bearer token, or "<anonymous>"
    QString   detail;          // free text; usually a short JSON blob
    QDateTime occurredAt;
};

class PackageRegistry {
public:
    explicit PackageRegistry(const QString &dataRoot);
    ~PackageRegistry();

    bool open(QString *error = nullptr);
    bool isOpen() const;

    // Optional HMAC-SHA256 signing key.  When non-empty, every publish() call
    // requires the package's manifest to carry a matching <Signature>; bytes
    // missing or mis-signed are rejected before disk is touched.  An empty
    // key keeps Core in permissive mode (the default).
    void       setSigningKey(const QByteArray &key) { m_signingKey = key; }
    QByteArray signingKey() const                   { return m_signingKey; }

    // Listener fired right after a package is persisted - used by the
    // CoreEntityStore to register the sheets so HTTP entity endpoints
    // can serve them immediately.  The listener receives the raw bytes
    // (the parsed Package is reconstructed by the listener if needed).
    using PackageListener = std::function<void(const QByteArray &bytes)>;
    void setPackageListener(PackageListener fn) { m_listener = std::move(fn); }

    // Persists `bytes` under <root>/packages/<id>/<version>.nexor and inserts
    // (or updates) the metadata row.  The .nexor is parsed via PackageReader
    // (built into Core via the studio source tree) so we can pull its title /
    // hash / built-at out of the manifest.  Returns the resulting record.
    bool publish(const QByteArray &bytes,
                 const QString   &actor,
                 RegistryRecord  &outRecord,
                 QString *error = nullptr);

    // Admin operations — flip a row's status.  Returns false if the row
    // doesn't exist or the requested transition is meaningless.  `actor`
    // is recorded in the audit trail.
    bool deploy        (const QString &id, const QString &version,
                        const QString &actor, QString *error = nullptr);
    bool rollback      (const QString &id, const QString &version,
                        const QString &actor, QString *error = nullptr);
    bool deletePending (const QString &id, const QString &version,
                        const QString &actor, QString *error = nullptr);

    QVector<RegistryRecord> list() const;
    QVector<RegistryRecord> list(const QString &statusFilter) const;
    QVector<RegistryRecord> listByPackage(const QString &id) const;
    bool                    find(const QString &id, const QString &version,
                                 RegistryRecord &out) const;
    QByteArray              read(const QString &id, const QString &version) const;

    // Audit log
    QVector<AuditEvent> audit(const QString &packageId = QString(),
                              int limit = 500) const;

    QString rootDir() const { return m_root; }

private:
    bool ensureSchema(QString *error);
    void recordAudit(const QString &eventType,
                     const QString &id, const QString &version,
                     const QString &actor, const QString &detail);

    QString          m_root;
    QSqlDatabase     m_db;
    QString          m_dbName;     // unique connection name
    QByteArray       m_signingKey;
    PackageListener  m_listener;
    bool             m_open { false };
};

} // namespace nx

#endif // NEXOR_CORE_PACKAGEREGISTRY_H
