#include "Process.h"
#include "BpmnIo.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

Process::Process() {
    // Every brand-new process gets a Start (Server) and an End (Final) step
    // wired Start -> End, so it executes cleanly out of the box.
    StepSpec start;
    start.id   = "Start";
    start.type = "Server";
    start.nextId = "End";
    start.code = "    ' Start step — runs first.\n"
                 "    Print \"Process started.\"\n";
    m_steps.append(start);

    StepSpec end;
    end.id   = "End";
    end.type = "Final";
    end.code = "    ' Final step — process terminates after this runs.\n"
               "    Print \"Process finished.\"\n";
    m_steps.append(end);
}

Process::Process(const ProcessMeta &meta) : Process() {
    m_meta = meta;
}

bool Process::isBpmn() const {
    return m_filePath.endsWith(".bpmn", Qt::CaseInsensitive);
}

int Process::indexOfStep(const QString &id) const {
    for (int i = 0; i < m_steps.size(); ++i)
        if (m_steps.at(i).id.compare(id, Qt::CaseInsensitive) == 0) return i;
    return -1;
}

int Process::startIndex() const {
    int i = indexOfStep("Start");
    if (i >= 0) return i;
    return m_steps.isEmpty() ? -1 : 0;
}

bool Process::save() const {
    if (m_filePath.isEmpty()) return false;
    QFileInfo fi(m_filePath);
    QDir().mkpath(fi.absolutePath());

    if (isBpmn()) {
        QByteArray bytes = nx::BpmnIo::writeBpmn(*this);
        QFile f(m_filePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        f.write(bytes);
        return true;
    }

    QFile f(m_filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QXmlStreamWriter w(&f);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("Process");
    w.writeAttribute("version", "1");

    w.writeStartElement("Meta");
    w.writeTextElement("Title",       m_meta.title);
    w.writeTextElement("Id",          m_meta.id);
    w.writeTextElement("Description", m_meta.description);
    w.writeTextElement("Author",      m_meta.author);
    w.writeTextElement("Created",     m_meta.created.toString(Qt::ISODate));
    w.writeEndElement();

    w.writeStartElement("Steps");
    for (const StepSpec &st : m_steps) {
        w.writeStartElement("Step");
        w.writeAttribute("id",   st.id);
        w.writeAttribute("type", st.type);
        if (!st.nextId.isEmpty())
            w.writeAttribute("next", st.nextId);
        if (!st.formId.isEmpty())
            w.writeAttribute("form", st.formId);
        w.writeCDATA(st.code);
        w.writeEndElement();
    }
    w.writeEndElement();

    w.writeEndElement(); // Process
    w.writeEndDocument();
    return true;
}

bool Process::load() {
    QFile f(m_filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray bytes = f.readAll();
    f.close();

    // BPMN 2.0 takes precedence — that's the canonical format.  Legacy .prc
    // files starting with <Process> still load, and Studio offers to upgrade
    // them on first save.
    if (nx::BpmnIo::sniff(bytes)) {
        QString err;
        bool ok = nx::BpmnIo::readBpmn(bytes, *this, &err);
        return ok;
    }

    QXmlStreamReader r(bytes);
    m_steps.clear();
    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement()) continue;
        const auto name = r.name();
        if      (name == "Title")       m_meta.title       = r.readElementText();
        else if (name == "Id")          m_meta.id          = r.readElementText();
        else if (name == "Description") m_meta.description = r.readElementText();
        else if (name == "Author")      m_meta.author      = r.readElementText();
        else if (name == "Created")     m_meta.created     = QDateTime::fromString(r.readElementText(), Qt::ISODate);
        else if (name == "Step") {
            const auto a = r.attributes();
            StepSpec st;
            st.id     = a.value("id").toString();
            st.type   = a.value("type").toString();
            if (st.type.isEmpty()) st.type = "Server";
            st.nextId = a.value("next").toString();
            st.formId = a.value("form").toString();
            st.code   = r.readElementText(); // CDATA flattens to text here
            if (!st.id.isEmpty()) m_steps.append(st);
        }
    }
    return !r.hasError();
}
