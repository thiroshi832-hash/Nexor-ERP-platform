#include "EntityStore.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QUuid>
#include <QDebug>

namespace nx {

// ─── SheetSchema helpers ────────────────────────────────────────────────
bool SheetSchema::hasField(const QString &name) const {
    QString lo = name.toLower();
    for (const auto &f : fields) if (f.name.toLower() == lo) return true;
    return false;
}
QString SheetSchema::keyField() const {
    for (const auto &f : fields) if (f.isKey) return f.name;
    return "Id";
}

// ─── Entity ─────────────────────────────────────────────────────────────
Entity::Entity(QString sheetId) : m_sheetId(std::move(sheetId)) {}

static QString lc(const QString &s) { return s.toLower(); }

bool  Entity::has(const QString &name) const { return m_fields.contains(lc(name)); }
Value Entity::get(const QString &name) const { return m_fields.value(lc(name)); }
void  Entity::set(const QString &name, const Value &v) {
    m_fields.insert(lc(name), v);
    if (lc(name) == "id") m_id = v.toLong();
}

QStringList Entity::fieldNames() const {
    QStringList out;
    for (auto it = m_fields.constBegin(); it != m_fields.constEnd(); ++it)
        out.append(it.key());
    out.sort();
    return out;
}

// ─── EntityTable ────────────────────────────────────────────────────────
EntityTable::EntityTable(QSqlDatabase db, SheetSchema schema)
    : m_db(std::move(db)), m_schema(std::move(schema)) {}

QString EntityTable::tableName() const { return m_schema.sheetId; }

// Map a Nexor type to a SQLite storage class.
QString EntityTable::sqlType(const QString &nexorType) {
    QString t = nexorType.toLower();
    if (t == "long" || t == "integer" || t == "boolean") return "INTEGER";
    if (t == "double")                                   return "REAL";
    if (t == "decimal")                                  return "REAL"; // Phase 6 will tighten
    return "TEXT";  // String, Date, Variant, anything else
}

QStringList EntityTable::columnsInDb() const {
    QSqlQuery q(m_db);
    if (!q.exec(QString("PRAGMA table_info(%1)").arg(tableName())))
        return {};
    QStringList cols;
    while (q.next()) cols << q.value(1).toString();   // column 1 = name
    return cols;
}

bool EntityTable::ensureSchema() {
    if (!m_db.isOpen()) return false;
    QSqlQuery q(m_db);

    // 1. Build CREATE TABLE.  Primary key is whichever field has isKey.
    //    We force INTEGER PRIMARY KEY AUTOINCREMENT for the key column so
    //    SQLite assigns ids automatically on INSERT.
    QString keyName;
    QStringList colDefs;
    for (const auto &f : m_schema.fields) {
        QString def = QString("\"%1\" %2").arg(f.name, sqlType(f.type));
        if (f.isKey && keyName.isEmpty()) {
            def = QString("\"%1\" INTEGER PRIMARY KEY AUTOINCREMENT").arg(f.name);
            keyName = f.name;
        } else if (f.required) {
            def += " NOT NULL";
        }
        colDefs << def;
    }
    if (keyName.isEmpty()) {
        // No explicit key — give every table a synthetic Id.
        colDefs.prepend("\"Id\" INTEGER PRIMARY KEY AUTOINCREMENT");
        keyName = "Id";
    }

    QString sqlCreate = QString("CREATE TABLE IF NOT EXISTS \"%1\" (%2)")
                          .arg(tableName(), colDefs.join(", "));
    if (!q.exec(sqlCreate)) {
        qWarning() << "ensureSchema CREATE failed:" << q.lastError().text();
        return false;
    }

    // 2. ALTER TABLE for any field that's in the schema but missing in DB.
    QStringList existing = columnsInDb();
    QStringList existingLo;
    for (const auto &c : existing) existingLo << c.toLower();

    for (const auto &f : m_schema.fields) {
        if (!existingLo.contains(f.name.toLower())) {
            QString sqlAlter = QString("ALTER TABLE \"%1\" ADD COLUMN \"%2\" %3")
                                 .arg(tableName(), f.name, sqlType(f.type));
            if (!q.exec(sqlAlter))
                qWarning() << "ensureSchema ALTER failed:" << q.lastError().text();
        }
    }
    return true;
}

std::shared_ptr<Entity> EntityTable::create() {
    auto e = std::make_shared<Entity>(m_schema.sheetId);
    for (const auto &f : m_schema.fields) {
        if (!f.defaultText.isEmpty())
            e->set(f.name, Value::text(f.defaultText));
    }
    return e;
}

void EntityTable::bindFromEntity(QSqlQuery &q,
                                 const std::shared_ptr<Entity> &e) const {
    for (const auto &f : m_schema.fields) {
        if (f.isKey && !e->isPersisted()) continue;     // let SQLite assign
        Value v = e->get(f.name);
        QVariant qv;
        if (!v.isEmpty()) {
            QString t = f.type.toLower();
            if      (t == "long" || t == "integer") qv = QVariant(v.toLong());
            else if (t == "boolean")                qv = QVariant(v.toBool() ? 1 : 0);
            else if (t == "double" || t == "decimal") qv = QVariant(v.toDouble());
            else                                    qv = QVariant(v.toText());
        }
        q.bindValue(":" + f.name, qv);
    }
}

bool EntityTable::save(std::shared_ptr<Entity> e) {
    if (!e || !m_db.isOpen()) return false;

    QStringList cols, placeholders, sets;
    for (const auto &f : m_schema.fields) {
        if (f.isKey && !e->isPersisted()) continue;     // skip pk on INSERT
        cols         << QString("\"%1\"").arg(f.name);
        placeholders << ":" + f.name;
        sets         << QString("\"%1\" = :%1").arg(f.name);
    }

    QSqlQuery q(m_db);
    if (!e->isPersisted()) {
        QString sql = QString("INSERT INTO \"%1\" (%2) VALUES (%3)")
                        .arg(tableName(), cols.join(", "), placeholders.join(", "));
        q.prepare(sql);
        bindFromEntity(q, e);
        if (!q.exec()) {
            qWarning() << "save INSERT failed:" << q.lastError().text() << sql;
            return false;
        }
        e->setId(q.lastInsertId().toLongLong());
        e->setPersisted(true);
    } else {
        QString key = m_schema.keyField();
        QString sql = QString("UPDATE \"%1\" SET %2 WHERE \"%3\" = :__id")
                        .arg(tableName(), sets.join(", "), key);
        q.prepare(sql);
        bindFromEntity(q, e);
        q.bindValue(":__id", QVariant(e->id()));
        if (!q.exec()) {
            qWarning() << "save UPDATE failed:" << q.lastError().text() << sql;
            return false;
        }
    }
    return true;
}

std::shared_ptr<Entity> EntityTable::rowToEntity(const QSqlQuery &q) const {
    auto e = std::make_shared<Entity>(m_schema.sheetId);
    QSqlRecord rec = q.record();
    for (int i = 0; i < rec.count(); ++i) {
        QString name = rec.fieldName(i);
        QVariant v   = q.value(i);
        // Find the schema field to know how to coerce.
        QString typeLo;
        for (const auto &f : m_schema.fields)
            if (f.name.compare(name, Qt::CaseInsensitive) == 0)
                { typeLo = f.type.toLower(); break; }

        if (v.isNull()) {
            e->set(name, Value());
        } else if (typeLo == "long" || typeLo == "integer") {
            e->set(name, Value::integer(v.toLongLong()));
        } else if (typeLo == "boolean") {
            e->set(name, Value::boolean(v.toLongLong() != 0));
        } else if (typeLo == "double" || typeLo == "decimal") {
            e->set(name, Value::real(v.toDouble()));
        } else {
            e->set(name, Value::text(v.toString()));
        }
    }
    e->setPersisted(true);
    return e;
}

std::shared_ptr<Entity> EntityTable::find(qint64 id) const {
    if (!m_db.isOpen()) return nullptr;
    QSqlQuery q(m_db);
    QString key = m_schema.keyField();
    q.prepare(QString("SELECT * FROM \"%1\" WHERE \"%2\" = :id LIMIT 1")
                .arg(tableName(), key));
    q.bindValue(":id", QVariant(id));
    if (!q.exec() || !q.next()) return nullptr;
    return rowToEntity(q);
}

bool EntityTable::remove(qint64 id) {
    if (!m_db.isOpen()) return false;
    QSqlQuery q(m_db);
    QString key = m_schema.keyField();
    q.prepare(QString("DELETE FROM \"%1\" WHERE \"%2\" = :id")
                .arg(tableName(), key));
    q.bindValue(":id", QVariant(id));
    return q.exec() && q.numRowsAffected() > 0;
}

QVector<std::shared_ptr<Entity>> EntityTable::all() const {
    QVector<std::shared_ptr<Entity>> out;
    if (!m_db.isOpen()) return out;
    QSqlQuery q(m_db);
    QString key = m_schema.keyField();
    if (!q.exec(QString("SELECT * FROM \"%1\" ORDER BY \"%2\"")
                  .arg(tableName(), key)))
        return out;
    while (q.next()) out.append(rowToEntity(q));
    return out;
}

qint64 EntityTable::count() const {
    if (!m_db.isOpen()) return 0;
    QSqlQuery q(m_db);
    if (!q.exec(QString("SELECT COUNT(*) FROM \"%1\"").arg(tableName())))
        return 0;
    if (!q.next()) return 0;
    return q.value(0).toLongLong();
}

// ─── EntityStore ────────────────────────────────────────────────────────
EntityStore::EntityStore() {
    // Every store gets its own SQL connection name to avoid clashes when
    // multiple Studio instances or test fixtures run side-by-side.
    m_connectionName = "nx_" + QUuid::createUuid().toString(QUuid::Id128);
}

EntityStore::~EntityStore() { close(); }

bool EntityStore::open(const QString &filePath) {
    close();
    QFileInfo fi(filePath);
    QDir().mkpath(fi.absolutePath());
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(filePath);
    if (!m_db.open()) {
        qWarning() << "EntityStore::open failed:" << m_db.lastError().text();
        return false;
    }
    QSqlQuery pragmas(m_db);
    pragmas.exec("PRAGMA foreign_keys = ON");
    return true;
}

bool EntityStore::isOpen() const { return m_db.isOpen(); }

void EntityStore::close() {
    m_tables.clear();
    if (m_db.isOpen()) m_db.close();
    if (QSqlDatabase::contains(m_connectionName))
        QSqlDatabase::removeDatabase(m_connectionName);
}

void EntityStore::registerSheet(const SheetSchema &s) {
    auto t = std::make_shared<EntityTable>(m_db, s);
    if (m_db.isOpen()) t->ensureSchema();
    m_tables[norm(s.sheetId)] = t;
}

void EntityStore::unregisterSheet(const QString &sheetId) {
    m_tables.remove(norm(sheetId));
}

bool EntityStore::hasSheet(const QString &sheetId) const {
    return m_tables.contains(norm(sheetId));
}

EntityTable *EntityStore::table(const QString &sheetId) {
    auto it = m_tables.find(norm(sheetId));
    return it == m_tables.end() ? nullptr : it.value().get();
}

const SheetSchema *EntityStore::schema(const QString &sheetId) const {
    auto it = m_tables.find(norm(sheetId));
    return it == m_tables.end() ? nullptr : &it.value()->schema();
}

} // namespace nx
