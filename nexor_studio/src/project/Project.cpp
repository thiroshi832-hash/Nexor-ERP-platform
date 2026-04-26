#include "Project.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

Project::Project()  = default;
Project::~Project() = default;

QString Project::rootDir() const {
    return QFileInfo(m_filePath).absolutePath();
}

std::shared_ptr<Activity> Project::createAtomicActivity(const ActivityMeta &meta, QString *errorOut) {
    if (meta.id.trimmed().isEmpty()) {
        if (errorOut) *errorOut = "Activity ID must not be empty.";
        return nullptr;
    }
    // Disallow duplicates
    for (const auto &a : m_atomicActivities) {
        if (a->meta().id.compare(meta.id, Qt::CaseInsensitive) == 0) {
            if (errorOut) *errorOut = QString("Activity '%1' already exists.").arg(meta.id);
            return nullptr;
        }
    }

    QString root = rootDir();
    if (root.isEmpty()) {
        if (errorOut) *errorOut = "Project must be saved before adding activities.";
        return nullptr;
    }

    QString actDir   = root + "/activities/" + meta.id;
    QString abaPath  = actDir + "/" + meta.id + ".aba";
    QString mainForm = meta.id + "_main.frm";
    QString frmPath  = actDir + "/" + mainForm;

    if (!QDir().mkpath(actDir)) {
        if (errorOut) *errorOut = "Could not create activity directory: " + actDir;
        return nullptr;
    }

    auto act = std::make_shared<Activity>(meta);
    act->setFilePath(abaPath);
    act->addForm(mainForm);
    // Default starter code: a Main sub the runtime invokes.
    act->setCode(QString("' %1 — generated %2\n"
                         "Sub Main()\n"
                         "    ' Atomic activity entry point\n"
                         "End Sub\n")
                    .arg(meta.title, meta.created.toString(Qt::ISODate)));

    if (!Activity::writeBlankForm(frmPath, meta.id + "_main")) {
        if (errorOut) *errorOut = "Could not write main form: " + frmPath;
        return nullptr;
    }
    if (!act->save()) {
        if (errorOut) *errorOut = "Could not write activity file: " + abaPath;
        return nullptr;
    }

    m_atomicActivities.append(act);
    save(); // persist project file
    return act;
}

bool Project::save() const {
    if (m_filePath.isEmpty()) return false;
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());
    QFile f(m_filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;

    QXmlStreamWriter w(&f);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("NexorProject");
    w.writeAttribute("version", "1");

    w.writeStartElement("Meta");
    w.writeTextElement("Title",       m_meta.title);
    w.writeTextElement("Id",          m_meta.id);
    w.writeTextElement("Description", m_meta.description);
    w.writeTextElement("Author",      m_meta.author);
    w.writeTextElement("Created",     m_meta.created.toString(Qt::ISODate));
    w.writeEndElement();

    auto writeStringList = [&](const QString &group, const QString &item, const QStringList &xs) {
        w.writeStartElement(group);
        for (const QString &x : xs) w.writeTextElement(item, x);
        w.writeEndElement();
    };

    writeStringList("Events",            "Event",   m_events);

    w.writeStartElement("AtomicActivities");
    for (const auto &a : m_atomicActivities) {
        w.writeStartElement("Activity");
        w.writeAttribute("id",   a->meta().id);
        w.writeAttribute("file", QDir(rootDir()).relativeFilePath(a->filePath()));
        w.writeEndElement();
    }
    w.writeEndElement();

    writeStringList("ProcessActivities", "Process",  m_processActivities);
    writeStringList("Sheets",            "Sheet",    m_sheets);
    writeStringList("Reports",           "Report",   m_reports);
    writeStringList("Resources",         "Resource", m_resources);

    w.writeEndElement(); // NexorProject
    w.writeEndDocument();
    return true;
}

bool Project::load() {
    QFile f(m_filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QXmlStreamReader r(&f);

    m_events.clear(); m_atomicActivities.clear(); m_processActivities.clear();
    m_sheets.clear(); m_reports.clear(); m_resources.clear();

    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement()) continue;
        const auto name = r.name();
        if      (name == "Title")       m_meta.title       = r.readElementText();
        else if (name == "Id")          m_meta.id          = r.readElementText();
        else if (name == "Description") m_meta.description = r.readElementText();
        else if (name == "Author")      m_meta.author      = r.readElementText();
        else if (name == "Created")     m_meta.created     = QDateTime::fromString(r.readElementText(), Qt::ISODate);
        else if (name == "Event")       m_events           << r.readElementText();
        else if (name == "Process")     m_processActivities<< r.readElementText();
        else if (name == "Sheet")       m_sheets           << r.readElementText();
        else if (name == "Report")      m_reports          << r.readElementText();
        else if (name == "Resource")    m_resources        << r.readElementText();
        else if (name == "Activity") {
            QString rel = r.attributes().value("file").toString();
            auto act = std::make_shared<Activity>();
            act->setFilePath(QDir(rootDir()).absoluteFilePath(rel));
            act->load();
            m_atomicActivities.append(act);
        }
    }
    return !r.hasError();
}

std::unique_ptr<Project> Project::createOnDisk(const ProjectMeta &meta,
                                               const QString &rootDir,
                                               QString *errorOut) {
    if (meta.id.trimmed().isEmpty()) {
        if (errorOut) *errorOut = "Project ID must not be empty.";
        return {};
    }
    QDir dir(rootDir);
    if (!dir.mkpath(".")) {
        if (errorOut) *errorOut = "Could not create project directory: " + rootDir;
        return {};
    }
    auto p = std::make_unique<Project>();
    p->setMeta(meta);
    p->setFilePath(dir.absoluteFilePath(meta.id + ".pro"));
    if (!p->save()) {
        if (errorOut) *errorOut = "Could not write project file.";
        return {};
    }
    return p;
}
