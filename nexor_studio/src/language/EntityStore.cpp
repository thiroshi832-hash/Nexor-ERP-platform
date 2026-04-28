#include "EntityStore.h"

namespace nx {

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

// ─── EntityTable ────────────────────────────────────────────────────────
EntityTable::EntityTable(SheetSchema schema) : m_schema(std::move(schema)) {}

std::shared_ptr<Entity> EntityTable::create() {
    auto e = std::make_shared<Entity>(m_schema.sheetId);
    // Apply field defaults
    for (const auto &f : m_schema.fields) {
        if (!f.defaultText.isEmpty()) {
            // crude type coercion using string default
            e->set(f.name, Value::text(f.defaultText));
        }
    }
    return e;
}

bool EntityTable::save(std::shared_ptr<Entity> e) {
    if (!e) return false;
    if (!e->isPersisted()) {
        e->setId(m_nextId++);
        e->setPersisted(true);
    }
    m_rows.insert(e->id(), e);
    return true;
}

std::shared_ptr<Entity> EntityTable::find(qint64 id) const {
    return m_rows.value(id, nullptr);
}

bool EntityTable::remove(qint64 id) {
    return m_rows.remove(id) > 0;
}

QVector<std::shared_ptr<Entity>> EntityTable::all() const {
    QVector<std::shared_ptr<Entity>> out;
    out.reserve(m_rows.size());
    auto keys = m_rows.keys();
    std::sort(keys.begin(), keys.end());
    for (qint64 k : keys) out.append(m_rows.value(k));
    return out;
}

// ─── EntityStore ────────────────────────────────────────────────────────
void EntityStore::registerSheet(const SheetSchema &s) {
    m_tables[norm(s.sheetId)] = std::make_shared<EntityTable>(s);
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
const SheetSchema* EntityStore::schema(const QString &sheetId) const {
    auto it = m_tables.find(norm(sheetId));
    return it == m_tables.end() ? nullptr : &it.value()->schema();
}

} // namespace nx
