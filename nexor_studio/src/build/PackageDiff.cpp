#include "PackageDiff.h"

#include <QCryptographicHash>
#include <QXmlStreamReader>
#include <QHash>

namespace nx {

namespace {

QString sha256Hex(const QString &content) {
    return QString::fromLatin1(
        QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Sha256)
            .toHex());
}

// Diffs one section's entries (Activities, Forms, Sheets, Processes).  Two
// entries match iff they share the same `id`; `Changed` is fired when they
// match but the content hash differs.
void diffSection(const QString &name,
                 const QVector<PackageEntry> &fromXs,
                 const QVector<PackageEntry> &toXs,
                 QVector<EntryChange> &out) {
    QHash<QString, const PackageEntry *> byIdFrom, byIdTo;
    for (const auto &e : fromXs) byIdFrom.insert(e.id, &e);
    for (const auto &e : toXs)   byIdTo  .insert(e.id, &e);
    for (const auto &e : toXs) {
        EntryChange c;
        c.section = name;
        c.id      = e.id;
        c.toHash  = sha256Hex(e.content);
        if (!byIdFrom.contains(e.id)) {
            c.kind = EntryChange::Added;
            out.append(c);
            continue;
        }
        const PackageEntry *from = byIdFrom.value(e.id);
        c.fromHash = sha256Hex(from->content);
        c.kind = (c.fromHash == c.toHash) ? EntryChange::Unchanged
                                          : EntryChange::Changed;
        out.append(c);
    }
    for (const auto &e : fromXs) {
        if (byIdTo.contains(e.id)) continue;
        EntryChange c;
        c.section  = name;
        c.id       = e.id;
        c.fromHash = sha256Hex(e.content);
        c.kind     = EntryChange::Removed;
        out.append(c);
    }
}

// One field row inside a Sheet's <Field …/> element.
struct FieldSpec {
    QString name, type, defaultText;
    bool isKey { false }, required { false };
};

// Pull <Field …/> rows out of a .sht-shaped XML body.  The format is the
// one written by Sheet::save in nexor_studio/src/project/Sheet.cpp.
QVector<FieldSpec> parseSheetFields(const QString &xml) {
    QVector<FieldSpec> out;
    QXmlStreamReader r(xml);
    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement() || r.name() != "Field") continue;
        const auto a = r.attributes();
        FieldSpec f;
        f.name        = a.value("name").toString();
        f.type        = a.value("type").toString();
        if (f.type.isEmpty()) f.type = "String";
        f.isKey       = a.value("key")     .toString() == "true";
        f.required    = a.value("required").toString() == "true";
        f.defaultText = a.value("default") .toString();
        if (!f.name.isEmpty()) out.append(f);
    }
    return out;
}

void diffSheetSchemas(const Package &fromPkg, const Package &toPkg,
                      QVector<SheetChange> &out) {
    QHash<QString, QString> fromXml, toXml;
    for (const auto &s : fromPkg.sheets) fromXml.insert(s.id, s.content);
    for (const auto &s : toPkg.sheets)   toXml  .insert(s.id, s.content);

    for (auto it = toXml.constBegin(); it != toXml.constEnd(); ++it) {
        if (!fromXml.contains(it.key())) continue;          // new sheet — caught at entry level
        QVector<FieldSpec> a = parseSheetFields(fromXml.value(it.key()));
        QVector<FieldSpec> b = parseSheetFields(it.value());
        QHash<QString, FieldSpec> aMap, bMap;
        for (const auto &f : a) aMap.insert(f.name.toLower(), f);
        for (const auto &f : b) bMap.insert(f.name.toLower(), f);

        SheetChange sc;
        sc.sheetId = it.key();
        for (const auto &fb : b) {
            QString k = fb.name.toLower();
            if (!aMap.contains(k)) {
                SheetFieldChange c;
                c.kind = SheetFieldChange::Added;
                c.fieldName = fb.name;
                c.toType    = fb.type;
                sc.fields.append(c);
                continue;
            }
            const FieldSpec fa = aMap.value(k);
            if (fa.type != fb.type) {
                SheetFieldChange c;
                c.kind      = SheetFieldChange::TypeChanged;
                c.fieldName = fb.name;
                c.fromType  = fa.type;
                c.toType    = fb.type;
                sc.fields.append(c);
            }
            if (fa.isKey != fb.isKey || fa.required != fb.required) {
                SheetFieldChange c;
                c.kind      = SheetFieldChange::FlagsChanged;
                c.fieldName = fb.name;
                QString d;
                if (fa.isKey != fb.isKey)
                    d += QString("key %1→%2 ").arg(fa.isKey ? "true" : "false",
                                                    fb.isKey ? "true" : "false");
                if (fa.required != fb.required)
                    d += QString("required %1→%2").arg(fa.required ? "true" : "false",
                                                        fb.required ? "true" : "false");
                c.detail    = d.trimmed();
                sc.fields.append(c);
            }
        }
        for (const auto &fa : a) {
            if (bMap.contains(fa.name.toLower())) continue;
            SheetFieldChange c;
            c.kind      = SheetFieldChange::Removed;
            c.fieldName = fa.name;
            c.fromType  = fa.type;
            sc.fields.append(c);
        }
        if (!sc.fields.isEmpty()) out.append(sc);
    }
}

} // namespace

int PackageDiff::counts(EntryChange::Kind k) const {
    int n = 0;
    for (const auto &c : entries) if (c.kind == k) ++n;
    return n;
}

PackageDiff PackageDiffer::compute(const Package &from, const Package &to) {
    PackageDiff d;
    diffSection("Activities", from.activities, to.activities, d.entries);
    diffSection("Forms",      from.forms,      to.forms,      d.entries);
    diffSection("Sheets",     from.sheets,     to.sheets,     d.entries);
    diffSection("Processes",  from.processes,  to.processes,  d.entries);
    {
        QVector<PackageEntry> fr, tr;
        for (const auto &r : from.resources) {
            PackageEntry e; e.id = r.file;
            e.content = QString::fromLatin1(r.data.toBase64());
            fr.append(e);
        }
        for (const auto &r : to.resources) {
            PackageEntry e; e.id = r.file;
            e.content = QString::fromLatin1(r.data.toBase64());
            tr.append(e);
        }
        diffSection("Resources", fr, tr, d.entries);
    }
    diffSheetSchemas(from, to, d.sheets);
    return d;
}

} // namespace nx
