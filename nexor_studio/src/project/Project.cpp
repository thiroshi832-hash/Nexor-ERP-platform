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

std::shared_ptr<Process> Project::createProcess(const ProcessMeta &meta, QString *errorOut) {
    if (meta.id.trimmed().isEmpty()) {
        if (errorOut) *errorOut = "Process ID must not be empty.";
        return nullptr;
    }
    for (const auto &p : m_processActivities) {
        if (p->meta().id.compare(meta.id, Qt::CaseInsensitive) == 0) {
            if (errorOut) *errorOut = QString("Process '%1' already exists.").arg(meta.id);
            return nullptr;
        }
    }
    QString root = rootDir();
    if (root.isEmpty()) {
        if (errorOut) *errorOut = "Project must be saved before adding processes.";
        return nullptr;
    }

    QString prcDir  = root + "/processes/" + meta.id;
    // BPMN 2.0 (.bpmn) is the canonical format from Phase 12 onward.  Any
    // existing .prc files still load via the legacy reader, so older
    // projects keep working unchanged.
    QString prcPath = prcDir + "/" + meta.id + ".bpmn";
    if (!QDir().mkpath(prcDir)) {
        if (errorOut) *errorOut = "Could not create process directory: " + prcDir;
        return nullptr;
    }
    auto prc = std::make_shared<Process>(meta);
    prc->setFilePath(prcPath);
    if (!prc->save()) {
        if (errorOut) *errorOut = "Could not write process file: " + prcPath;
        return nullptr;
    }
    m_processActivities.append(prc);
    save();
    return prc;
}

std::shared_ptr<Sheet> Project::createSheet(const SheetMeta &meta, QString *errorOut) {
    if (meta.id.trimmed().isEmpty()) {
        if (errorOut) *errorOut = "Sheet ID must not be empty.";
        return nullptr;
    }
    for (const auto &s : m_sheets) {
        if (s->meta().id.compare(meta.id, Qt::CaseInsensitive) == 0) {
            if (errorOut) *errorOut = QString("Sheet '%1' already exists.").arg(meta.id);
            return nullptr;
        }
    }
    QString root = rootDir();
    if (root.isEmpty()) {
        if (errorOut) *errorOut = "Project must be saved before adding sheets.";
        return nullptr;
    }

    QString shtDir  = root + "/sheets/" + meta.id;
    QString shtPath = shtDir + "/" + meta.id + ".sht";
    if (!QDir().mkpath(shtDir)) {
        if (errorOut) *errorOut = "Could not create sheet directory: " + shtDir;
        return nullptr;
    }
    auto sht = std::make_shared<Sheet>(meta);
    sht->setFilePath(shtPath);
    if (!sht->save()) {
        if (errorOut) *errorOut = "Could not write sheet file: " + shtPath;
        return nullptr;
    }
    m_sheets.append(sht);
    save();
    return sht;
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

    w.writeStartElement("ProcessActivities");
    for (const auto &p : m_processActivities) {
        w.writeStartElement("Process");
        w.writeAttribute("id",   p->meta().id);
        w.writeAttribute("file", QDir(rootDir()).relativeFilePath(p->filePath()));
        w.writeEndElement();
    }
    w.writeEndElement();

    w.writeStartElement("Sheets");
    for (const auto &s : m_sheets) {
        w.writeStartElement("Sheet");
        w.writeAttribute("id",   s->meta().id);
        w.writeAttribute("file", QDir(rootDir()).relativeFilePath(s->filePath()));
        w.writeEndElement();
    }
    w.writeEndElement();

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
        else if (name == "Report")      m_reports          << r.readElementText();
        else if (name == "Resource")    m_resources        << r.readElementText();
        else if (name == "Activity") {
            QString rel = r.attributes().value("file").toString();
            auto act = std::make_shared<Activity>();
            act->setFilePath(QDir(rootDir()).absoluteFilePath(rel));
            act->load();
            m_atomicActivities.append(act);
        }
        else if (name == "Process" && r.attributes().hasAttribute("file")) {
            QString rel = r.attributes().value("file").toString();
            auto prc = std::make_shared<Process>();
            prc->setFilePath(QDir(rootDir()).absoluteFilePath(rel));
            prc->load();
            m_processActivities.append(prc);
        }
        else if (name == "Sheet" && r.attributes().hasAttribute("file")) {
            // New format: Sheet element references a .sht file.
            QString rel = r.attributes().value("file").toString();
            auto sht = std::make_shared<Sheet>();
            sht->setFilePath(QDir(rootDir()).absoluteFilePath(rel));
            sht->load();
            m_sheets.append(sht);
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
