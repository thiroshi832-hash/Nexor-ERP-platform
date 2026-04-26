#include "Activity.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

Activity::Activity() = default;

Activity::Activity(const ActivityMeta &meta) : m_meta(meta) {}

bool Activity::save() const {
    if (m_filePath.isEmpty()) return false;
    QFileInfo fi(m_filePath);
    QDir().mkpath(fi.absolutePath());

    QFile f(m_filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;

    QXmlStreamWriter w(&f);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("AtomicActivity");
    w.writeAttribute("version", "1");

    w.writeStartElement("Meta");
    w.writeTextElement("Title",       m_meta.title);
    w.writeTextElement("Id",          m_meta.id);
    w.writeTextElement("Description", m_meta.description);
    w.writeTextElement("Author",      m_meta.author);
    w.writeTextElement("Created",     m_meta.created.toString(Qt::ISODate));
    w.writeEndElement(); // Meta

    w.writeStartElement("Forms");
    for (const QString &form : m_forms) {
        w.writeStartElement("Form");
        w.writeAttribute("file", form);
        w.writeEndElement();
    }
    w.writeEndElement(); // Forms

    w.writeStartElement("Code");
    w.writeCDATA(m_code);
    w.writeEndElement(); // Code

    w.writeEndElement(); // AtomicActivity
    w.writeEndDocument();
    return true;
}

bool Activity::load() {
    QFile f(m_filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QXmlStreamReader r(&f);
    m_forms.clear();
    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement()) continue;
        const auto name = r.name();
        if      (name == "Title")       m_meta.title       = r.readElementText();
        else if (name == "Id")          m_meta.id          = r.readElementText();
        else if (name == "Description") m_meta.description = r.readElementText();
        else if (name == "Author")      m_meta.author      = r.readElementText();
        else if (name == "Created")     m_meta.created     = QDateTime::fromString(r.readElementText(), Qt::ISODate);
        else if (name == "Form")        m_forms << r.attributes().value("file").toString();
        else if (name == "Code")        m_code             = r.readElementText();
    }
    return !r.hasError();
}

bool Activity::writeBlankForm(const QString &filePath, const QString &formId) {
    QFileInfo fi(filePath);
    QDir().mkpath(fi.absolutePath());
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;

    QXmlStreamWriter w(&f);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("Form");
    w.writeAttribute("version", "1");
    w.writeAttribute("id", formId);
    w.writeStartElement("Geometry");
    w.writeAttribute("x", "100");
    w.writeAttribute("y", "100");
    w.writeAttribute("width",  "640");
    w.writeAttribute("height", "480");
    w.writeEndElement();
    w.writeStartElement("Title");
    w.writeCharacters(formId);
    w.writeEndElement();
    w.writeStartElement("Widgets"); // empty designer canvas
    w.writeEndElement();
    w.writeEndElement(); // Form
    w.writeEndDocument();
    return true;
}
