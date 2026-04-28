#include "CoreEntityStore.h"

#include <QDir>
#include <QXmlStreamReader>

namespace nx {

CoreEntityStore::CoreEntityStore(const QString &root) : m_root(root) {}

bool CoreEntityStore::open(QString *error) {
    if (!QDir().mkpath(m_root)) {
        if (error) *error = "Could not create data root: " + m_root;
        return false;
    }
    QString dbPath = QDir(m_root).absoluteFilePath("core_entities.db");
    if (!m_inner.open(dbPath)) {
        if (error) *error = "Could not open " + dbPath;
        return false;
    }
    return true;
}

void CoreEntityStore::registerSheetsFromPackage(const Package &pkg) {
    for (const PackageEntry &se : pkg.sheets) {
        SheetSchema schema;
        schema.sheetId = se.id;
        QXmlStreamReader r(se.content);
        while (!r.atEnd()) {
            r.readNext();
            if (!r.isStartElement() || r.name() != "Field") continue;
            SheetSchemaField f;
            const auto a = r.attributes();
            f.name        = a.value("name").toString();
            f.type        = a.value("type").toString();
            if (f.type.isEmpty()) f.type = "String";
            f.isKey       = a.value("key").toString() == "true";
            f.required    = a.value("required").toString() == "true";
            f.defaultText = a.value("default").toString();
            if (!f.name.isEmpty()) schema.fields.append(f);
        }
        if (!schema.fields.isEmpty())
            m_inner.registerSheet(schema);
    }
}

} // namespace nx
