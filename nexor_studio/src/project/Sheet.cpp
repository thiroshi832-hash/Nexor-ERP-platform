#include "Sheet.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

Sheet::Sheet() {
    // Every sheet has at least an Id primary key.
    FieldSpec id;
    id.name = "Id"; id.type = "Long"; id.isKey = true; id.required = true;
    m_fields.append(id);
}

Sheet::Sheet(const SheetMeta &meta) : Sheet() {
    m_meta = meta;
}

bool Sheet::save() const {
    if (m_filePath.isEmpty()) return false;
    QFileInfo fi(m_filePath);
    QDir().mkpath(fi.absolutePath());

    QFile f(m_filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QXmlStreamWriter w(&f);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("Sheet");
    w.writeAttribute("version", "1");

    w.writeStartElement("Meta");
    w.writeTextElement("Title",       m_meta.title);
    w.writeTextElement("Id",          m_meta.id);
    w.writeTextElement("Description", m_meta.description);
    w.writeTextElement("Author",      m_meta.author);
    w.writeTextElement("Created",     m_meta.created.toString(Qt::ISODate));
    w.writeEndElement();

    w.writeStartElement("Fields");
    for (const FieldSpec &fs : m_fields) {
        w.writeStartElement("Field");
        w.writeAttribute("name", fs.name);
        w.writeAttribute("type", fs.type);
        if (fs.isKey)    w.writeAttribute("key",      "true");
        if (fs.required) w.writeAttribute("required", "true");
        if (!fs.defaultText.isEmpty())
            w.writeAttribute("default", fs.defaultText);
        w.writeEndElement();
    }
    w.writeEndElement();

    w.writeEndElement(); // Sheet
    w.writeEndDocument();
    return true;
}

bool Sheet::load() {
    QFile f(m_filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QXmlStreamReader r(&f);
    m_fields.clear();
    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement()) continue;
        const auto name = r.name();
        if      (name == "Title")       m_meta.title       = r.readElementText();
        else if (name == "Id")          m_meta.id          = r.readElementText();
        else if (name == "Description") m_meta.description = r.readElementText();
        else if (name == "Author")      m_meta.author      = r.readElementText();
        else if (name == "Created")     m_meta.created     = QDateTime::fromString(r.readElementText(), Qt::ISODate);
        else if (name == "Field") {
            const auto a = r.attributes();
            FieldSpec fs;
            fs.name        = a.value("name").toString();
            fs.type        = a.value("type").toString();
            if (fs.type.isEmpty()) fs.type = "String";
            fs.isKey       = a.value("key")     .toString() == "true";
            fs.required    = a.value("required").toString() == "true";
            fs.defaultText = a.value("default") .toString();
            m_fields.append(fs);
        }
    }
    if (m_fields.isEmpty()) {
        FieldSpec id;
        id.name = "Id"; id.type = "Long"; id.isKey = true; id.required = true;
        m_fields.append(id);
    }
    return !r.hasError();
}
