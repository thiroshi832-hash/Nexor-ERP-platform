#include "PackageRegistry.h"
#include "../../nexor_studio/src/build/Package.h"
#include "../../nexor_studio/src/build/PackageReader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDateTime>
#include <QDebug>

namespace nx {

PackageRegistry::PackageRegistry(const QString &dataRoot)
    : m_root(dataRoot),
      m_dbName(QString("nexor_core_%1")
                   .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))) {}

PackageRegistry::~PackageRegistry() {
    if (m_db.isOpen()) m_db.close();
    QSqlDatabase::removeDatabase(m_dbName);
}

bool PackageRegistry::isOpen() const { return m_open; }

bool PackageRegistry::open(QString *error) {
    if (!QDir().mkpath(m_root)) {
        if (error) *error = "Could not create data root: " + m_root;
        return false;
    }
    QString dbPath = QDir(m_root).absoluteFilePath("core.db");
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_dbName);
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        if (error) *error = "Could not open SQLite: " + m_db.lastError().text();
        return false;
    }
    if (!ensureSchema(error)) return false;
    m_open = true;
    qInfo().noquote() << "Registry data root:" << m_root;
    qInfo().noquote() << "Registry database :" << dbPath;
    return true;
}

bool PackageRegistry::ensureSchema(QString *error) {
    QSqlQuery q(m_db);
    const QString ddl =
        "CREATE TABLE IF NOT EXISTS package_versions ("
        "  id          TEXT NOT NULL,"
        "  version     TEXT NOT NULL,"
        "  title       TEXT,"
        "  built_at    TEXT,"
        "  hash        TEXT,"
        "  hash_algo   TEXT,"
        "  byte_size   INTEGER,"
        "  received_at TEXT,"
        "  file_path   TEXT NOT NULL,"
        "  status      TEXT NOT NULL,"
        "  PRIMARY KEY (id, version)"
        ")";
    if (!q.exec(ddl)) {
        if (error) *error = "DDL failed: " + q.lastError().text();
        return false;
    }
    return true;
}

bool PackageRegistry::publish(const QByteArray &bytes,
                              RegistryRecord &outRecord,
                              QString *error) {
    if (!m_open) { if (error) *error = "Registry not open."; return false; }
    if (bytes.isEmpty()) { if (error) *error = "Empty payload."; return false; }

    auto rd = PackageReader::fromBytes(bytes, /*verifyHash*/true);
    if (rd.status == PackageReader::Status::ParseError) {
        if (error) *error = "Bad package: " + rd.message;
        return false;
    }
    if (rd.status == PackageReader::Status::HashMismatch) {
        if (error) *error = "Hash mismatch: " + rd.message;
        return false;
    }
    if (rd.package.meta.id.isEmpty() || rd.package.meta.version.isEmpty()) {
        if (error) *error = "Manifest missing Id or Version.";
        return false;
    }

    // Signature check — when Core was started with --signing-key, every
    // upload must carry a matching <Signature>.  Without a key Core stays
    // permissive (the default for dev / first-run).
    if (!m_signingKey.isEmpty()) {
        QString sigMsg;
        auto sst = PackageReader::verifySignature(rd.package, m_signingKey, &sigMsg);
        if (sst != PackageReader::Status::Ok) {
            if (error) *error = (sst == PackageReader::Status::SignatureMissing)
                ? "Package is unsigned but Core requires signed uploads."
                : ("Signature mismatch: " + sigMsg);
            return false;
        }
    }

    // Write bytes to <root>/packages/<id>/<version>.nexor
    QString pkgDir = QDir(m_root).absoluteFilePath("packages/" + rd.package.meta.id);
    if (!QDir().mkpath(pkgDir)) {
        if (error) *error = "Could not create package directory: " + pkgDir;
        return false;
    }
    QString filePath = QDir(pkgDir).absoluteFilePath(rd.package.meta.version + ".nexor");
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = "Could not write package: " + filePath;
        return false;
    }
    f.write(bytes);
    f.close();

    // Upsert metadata row.
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO package_versions"
        " (id, version, title, built_at, hash, hash_algo, byte_size,"
        "  received_at, file_path, status)"
        " VALUES (:id,:ver,:title,:built,:hash,:halgo,:size,:recv,:path,'pending')"
        " ON CONFLICT(id, version) DO UPDATE SET"
        "   title=excluded.title, built_at=excluded.built_at,"
        "   hash=excluded.hash,   hash_algo=excluded.hash_algo,"
        "   byte_size=excluded.byte_size,"
        "   received_at=excluded.received_at,"
        "   file_path=excluded.file_path");
    QDateTime now = QDateTime::currentDateTimeUtc();
    q.bindValue(":id",    rd.package.meta.id);
    q.bindValue(":ver",   rd.package.meta.version);
    q.bindValue(":title", rd.package.meta.title);
    q.bindValue(":built", rd.package.meta.builtAt.toUTC().toString(Qt::ISODate));
    q.bindValue(":hash",  rd.package.meta.hash);
    q.bindValue(":halgo", rd.package.meta.hashAlgo);
    q.bindValue(":size",  static_cast<qint64>(bytes.size()));
    q.bindValue(":recv",  now.toString(Qt::ISODate));
    q.bindValue(":path",  filePath);
    if (!q.exec()) {
        if (error) *error = "DB upsert failed: " + q.lastError().text();
        return false;
    }

    outRecord.id         = rd.package.meta.id;
    outRecord.version    = rd.package.meta.version;
    outRecord.title      = rd.package.meta.title;
    outRecord.builtAt    = rd.package.meta.builtAt;
    outRecord.hash       = rd.package.meta.hash;
    outRecord.hashAlgo   = rd.package.meta.hashAlgo;
    outRecord.byteSize   = bytes.size();
    outRecord.receivedAt = now;
    outRecord.filePath   = filePath;
    outRecord.status     = "pending";
    return true;
}

QVector<RegistryRecord> PackageRegistry::list() const {
    return list(QString());
}

QVector<RegistryRecord> PackageRegistry::list(const QString &statusFilter) const {
    QVector<RegistryRecord> out;
    if (!m_open) return out;
    QSqlQuery q(m_db);
    QString sql =
        "SELECT id, version, title, built_at, hash, hash_algo, byte_size,"
        "       received_at, file_path, status"
        "  FROM package_versions";
    if (!statusFilter.isEmpty()) sql += " WHERE status = :status";
    sql += " ORDER BY received_at DESC, id ASC, version ASC";
    q.prepare(sql);
    if (!statusFilter.isEmpty()) q.bindValue(":status", statusFilter);
    if (!q.exec()) return out;
    while (q.next()) {
        RegistryRecord r;
        r.id         = q.value(0).toString();
        r.version    = q.value(1).toString();
        r.title      = q.value(2).toString();
        r.builtAt    = QDateTime::fromString(q.value(3).toString(), Qt::ISODate);
        r.hash       = q.value(4).toString();
        r.hashAlgo   = q.value(5).toString();
        r.byteSize   = q.value(6).toLongLong();
        r.receivedAt = QDateTime::fromString(q.value(7).toString(), Qt::ISODate);
        r.filePath   = q.value(8).toString();
        r.status     = q.value(9).toString();
        out.append(r);
    }
    return out;
}

bool PackageRegistry::deploy(const QString &id, const QString &version,
                             QString *error) {
    if (!m_open) { if (error) *error = "Registry not open."; return false; }
    RegistryRecord cur;
    if (!find(id, version, cur)) {
        if (error) *error = "No such package version.";
        return false;
    }
    QSqlQuery q(m_db);
    // Demote any previously-live row of the same id to 'rolled_back', then
    // promote this one to 'live'.  Two-step because SQLite can't UPDATE …
    // FROM and we want a sane history trail.
    q.prepare("UPDATE package_versions SET status='rolled_back'"
              " WHERE id=:id AND status='live' AND version != :ver");
    q.bindValue(":id", id);
    q.bindValue(":ver", version);
    if (!q.exec()) {
        if (error) *error = "Demote step failed: " + q.lastError().text();
        return false;
    }
    QSqlQuery up(m_db);
    up.prepare("UPDATE package_versions SET status='live'"
               " WHERE id=:id AND version=:ver");
    up.bindValue(":id", id);
    up.bindValue(":ver", version);
    if (!up.exec()) {
        if (error) *error = "Promote step failed: " + up.lastError().text();
        return false;
    }
    return true;
}

bool PackageRegistry::rollback(const QString &id, const QString &version,
                               QString *error) {
    if (!m_open) { if (error) *error = "Registry not open."; return false; }
    RegistryRecord cur;
    if (!find(id, version, cur)) {
        if (error) *error = "No such package version.";
        return false;
    }
    if (cur.status != "live") {
        if (error) *error = "Only live packages can be rolled back.";
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare("UPDATE package_versions SET status='rolled_back'"
              " WHERE id=:id AND version=:ver");
    q.bindValue(":id", id);
    q.bindValue(":ver", version);
    if (!q.exec()) {
        if (error) *error = "Rollback failed: " + q.lastError().text();
        return false;
    }
    return true;
}

bool PackageRegistry::find(const QString &id, const QString &version,
                           RegistryRecord &out) const {
    if (!m_open) return false;
    QSqlQuery q(m_db);
    q.prepare("SELECT id, version, title, built_at, hash, hash_algo, byte_size,"
              "       received_at, file_path, status"
              "  FROM package_versions WHERE id=:id AND version=:ver");
    q.bindValue(":id", id);
    q.bindValue(":ver", version);
    if (!q.exec() || !q.next()) return false;
    out.id         = q.value(0).toString();
    out.version    = q.value(1).toString();
    out.title      = q.value(2).toString();
    out.builtAt    = QDateTime::fromString(q.value(3).toString(), Qt::ISODate);
    out.hash       = q.value(4).toString();
    out.hashAlgo   = q.value(5).toString();
    out.byteSize   = q.value(6).toLongLong();
    out.receivedAt = QDateTime::fromString(q.value(7).toString(), Qt::ISODate);
    out.filePath   = q.value(8).toString();
    out.status     = q.value(9).toString();
    return true;
}

QByteArray PackageRegistry::read(const QString &id, const QString &version) const {
    RegistryRecord r;
    if (!find(id, version, r)) return {};
    QFile f(r.filePath);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return f.readAll();
}

} // namespace nx
