// =============================================================================
// Sheet — an entity-schema definition (.sht file).
//
// A Sheet declares the fields of a business object that the runtime stores
// (in memory now, in Postgres on Core later) and that the language exposes
// as a typed entity:
//
//     Customer.New()       -> creates a new entity instance
//     Customer.Find(id)    -> looks up by primary key
//     Customer.All()       -> returns the list of all rows
//     entity.FieldName     -> reads a field
//     entity.FieldName = X -> writes a field
//     entity.Save()        -> persists
//     entity.Delete()      -> removes
//
// Disk layout:  activities/<n/a>;  sheets live in   sheets/<id>/<id>.sht
// =============================================================================
#ifndef NEXOR_STUDIO_SHEET_H
#define NEXOR_STUDIO_SHEET_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QVector>

struct SheetMeta {
    QString   title;
    QString   id;
    QString   description;
    QString   author;
    QDateTime created;
};

// One field in the schema.  Type names are language-level (String, Long, Decimal,
// Double, Date, Boolean) — the runtime decides storage.
struct FieldSpec {
    QString name;
    QString type        { "String" };
    bool    isKey       { false };          // primary key
    bool    required    { false };
    QString defaultText;                    // string form of default value

    // Reserved for later: pattern, maxLength, computed, audit, foreign-key
};

class Sheet {
public:
    Sheet();
    explicit Sheet(const SheetMeta &meta);

    const SheetMeta &meta() const     { return m_meta; }
    void  setMeta(const SheetMeta &m) { m_meta = m; }

    QString filePath() const                  { return m_filePath; }
    void    setFilePath(const QString &p)     { m_filePath = p; }

    QVector<FieldSpec>&       fields()        { return m_fields; }
    const QVector<FieldSpec>& fields() const  { return m_fields; }

    // XML I/O — serialises {meta, fields} into a .sht file.
    bool save() const;
    bool load();

private:
    SheetMeta            m_meta;
    QString              m_filePath;
    QVector<FieldSpec>   m_fields;
};

#endif // NEXOR_STUDIO_SHEET_H
