// =============================================================================
// EntityStore — in-memory runtime entity storage.
//
//   • SheetSchema mirrors a project Sheet's field list at runtime.
//   • Entity is a row: a hash of field name → Value.  Has an Id (auto-incremented
//     by its store).
//   • EntityTable holds all rows for one sheet, indexed by Id.
//   • EntityStore owns one EntityTable per registered sheet.
//   • SheetRef is a runtime handle to a sheet (returned when user code says
//     `Customer`); it carries pointers back into the store so that .New / .Find
//     / .All / .Save calls find their data.
//
// This is in-memory only — Phase 4 will wire it to SQLite, Phase 6 to Postgres
// on Nexor Core.
// =============================================================================
#ifndef NEXOR_STUDIO_LANG_ENTITYSTORE_H
#define NEXOR_STUDIO_LANG_ENTITYSTORE_H

#include "Value.h"
#include <QString>
#include <QVector>
#include <QHash>
#include <memory>

namespace nx {

struct SheetSchemaField {
    QString name;
    QString type;        // "String", "Long", "Double", "Decimal", "Boolean", "Date"
    bool    isKey       { false };
    bool    required    { false };
    QString defaultText;
};

struct SheetSchema {
    QString                    sheetId;
    QVector<SheetSchemaField>  fields;
    bool hasField(const QString &name) const;
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
    explicit EntityTable(SheetSchema schema);

    const SheetSchema& schema() const { return m_schema; }

    std::shared_ptr<Entity> create();                   // unsaved, no id
    bool   save(std::shared_ptr<Entity> e);             // assigns id if new
    std::shared_ptr<Entity> find(qint64 id) const;
    bool   remove(qint64 id);
    QVector<std::shared_ptr<Entity>> all() const;

private:
    SheetSchema                                  m_schema;
    qint64                                       m_nextId { 1 };
    QHash<qint64, std::shared_ptr<Entity>>       m_rows;
};

class EntityStore {
public:
    void registerSheet(const SheetSchema &s);
    void unregisterSheet(const QString &sheetId);
    bool hasSheet(const QString &sheetId) const;

    EntityTable* table(const QString &sheetId);              // null if unknown
    const SheetSchema* schema(const QString &sheetId) const;

    QStringList sheetIds() const { return m_tables.keys(); }

private:
    QHash<QString, std::shared_ptr<EntityTable>> m_tables;   // keys lowercased
    static QString norm(const QString &s) { return s.toLower(); }
};

// SheetRef — runtime handle that the interpreter exposes via the sheet name.
struct SheetRef {
    QString      sheetId;     // canonical (case-preserved) name
    EntityStore *store;       // weak pointer; lifetime owned by Interpreter
};

} // namespace nx

#endif // NEXOR_STUDIO_LANG_ENTITYSTORE_H
