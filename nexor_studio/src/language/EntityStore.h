// =============================================================================
// EntityStore — SQLite-backed runtime entity storage.
//
// Every project keeps a single SQLite file (project.ndb in the project root).
// One SQL table per registered sheet, columns mirror the FieldSpec list.
// All entity operations (.New, .Find, .All, .Count, .Save, .Delete) are
// translated to prepared SQL statements with bound parameters.
//
// Ownership model:
//   • EntityStore owns the QSqlDatabase connection (named per project).
//   • EntityTable holds a copy of the schema + a pointer to the connection.
//   • Entity is a transient row object (hash of field name → Value) that
//     either represents an unsaved row (id == 0) or a persisted one.
//
// Schema migration is conservative: we CREATE TABLE IF NOT EXISTS on register,
// and ALTER TABLE ADD COLUMN for any new field that isn't already present.
// We never DROP a column (SQLite < 3.35 can't anyway) or change its type —
// such changes need an explicit migration story we'll add in a later phase.
// =============================================================================
#ifndef NEXOR_STUDIO_LANG_ENTITYSTORE_H
#define NEXOR_STUDIO_LANG_ENTITYSTORE_H

#include "Value.h"
#include <QString>
#include <QStringList>
#include <QVector>
#include <QHash>
#include <QSqlDatabase>
#include <functional>
#include <memory>

namespace nx {

class EntityStore;

struct SheetSchemaField {
    QString name;
    QString type;        // "String", "Long", "Integer", "Double", "Decimal",
                         // "Boolean", "Date", "Variant"
    bool    isKey       { false };
    bool    required    { false };
    QString defaultText;
};

struct SheetSchema {
    QString                    sheetId;
    QVector<SheetSchemaField>  fields;
    bool    hasField(const QString &name) const;
    QString keyField() const;            // first key field, or "Id" by default
};

class Entity {
public:
    explicit Entity(QString sheetId);
    QString sheetId() const           { return m_sheetId; }
    qint64  id() const                { return m_id; }
    void    setId(qint64 v)           { m_id = v; m_fields.insert("id", Value::integer(v)); }

    bool    has(const QString &name) const;
    Value   get(const QString &name) const;
    void    set(const QString &name, const Value &v);

    // Field names actually present on this entity (lowercase).  Used by
    // the JSON serialisation layer when packaging an Entity for RPC.
    QStringList fieldNames() const;

    bool    isPersisted() const       { return m_persisted; }
    void    setPersisted(bool b)      { m_persisted = b; }

private:
    QString               m_sheetId;
    qint64                m_id { 0 };
    QHash<QString, Value> m_fields;     // keys lower-cased
    bool                  m_persisted { false };
};

class EntityTable {
public:
    EntityTable() = default;
    EntityTable(QSqlDatabase db, SheetSchema schema, EntityStore *owner = nullptr);

    const SheetSchema& schema() const { return m_schema; }

    // Brings the SQL table in line with the schema.  Idempotent.
    bool ensureSchema();

    // CRUD
    std::shared_ptr<Entity> create();                   // unsaved, no id
    bool   save(std::shared_ptr<Entity> e);             // INSERT or UPDATE
    std::shared_ptr<Entity> find(qint64 id) const;
    bool   remove(qint64 id);
    QVector<std::shared_ptr<Entity>> all() const;
    qint64 count() const;

private:
    static QString sqlType(const QString &nexorType);
    QString tableName() const;
    QStringList columnsInDb() const;
    std::shared_ptr<Entity> rowToEntity(const class QSqlQuery &q) const;
    void    bindFromEntity(class QSqlQuery &q, const std::shared_ptr<Entity> &e) const;

    QSqlDatabase   m_db;
    SheetSchema    m_schema;
    EntityStore   *m_owner { nullptr };
};

class EntityStore {
public:
    using ErrorSink = std::function<void(const QString &)>;

    EntityStore();
    ~EntityStore();

    // Register a callback that will receive user-visible diagnostic messages
    // (open failures, INSERT/UPDATE failures, schema migration errors).
    // Studio wires this to the runtime error pane so silent SQL failures
    // become red lines in the output dock instead of disappearing into
    // qWarning/stderr.  Callable on EntityTable too via reportError().
    void setErrorSink(ErrorSink cb) { m_onError = std::move(cb); }
    void reportError(const QString &msg) const;

    // Connects (or creates) the SQLite file at filePath.  Subsequent
    // registerSheet calls run schema migrations against this database.
    bool open(const QString &filePath);
    bool isOpen() const;
    void close();

    // Register / unregister the runtime view of a sheet.  Adding a sheet
    // that already exists in the DB is a no-op; adding new fields runs
    // ALTER TABLE ADD COLUMN.
    void registerSheet(const SheetSchema &s);
    void unregisterSheet(const QString &sheetId);
    bool hasSheet(const QString &sheetId) const;

    EntityTable*       table(const QString &sheetId);
    const SheetSchema* schema(const QString &sheetId) const;

    QStringList sheetIds() const { return m_tables.keys(); }

private:
    static QString norm(const QString &s) { return s.toLower(); }

    QString                                      m_connectionName;
    QSqlDatabase                                 m_db;
    QHash<QString, std::shared_ptr<EntityTable>> m_tables;   // keys lowercased
    ErrorSink                                    m_onError;
};

// SheetRef — runtime handle the interpreter exposes via the sheet name.
struct SheetRef {
    QString      sheetId;     // canonical (case-preserved) name
    EntityStore *store;       // weak pointer; lifetime owned by Interpreter
};

} // namespace nx

#endif // NEXOR_STUDIO_LANG_ENTITYSTORE_H
