#include "PackageCache.h"

#include "../../nexor_studio/src/build/Package.h"
#include "../../nexor_studio/src/build/PackageReader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDateTime>

namespace nx {

PackageCache::PackageCache(const QString &root) : m_root(root) {}

bool PackageCache::prepare(QString *error) {
    QDir d;
    if (!d.mkpath(packagesDir()) || !d.mkpath(extractedDir())) {
        if (error) *error = "Could not create cache: " + m_root;
        return false;
    }
    return true;
}

QString PackageCache::packagePathFor(const QString &id,
                                     const QString &version) const {
    return QString("%1/%2/%3.nexor").arg(packagesDir(), id, version);
}

QString PackageCache::sniffTitle(const QString &nexorPath) {
    QFile f(nexorPath);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QXmlStreamReader r(&f);
    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement() && r.name() == "Title")
            return r.readElementText();
    }
    return {};
}

QVector<InstalledRow> PackageCache::list() const {
    QVector<InstalledRow> out;
    QDir packs(packagesDir());
    if (!packs.exists()) return out;
    for (const QString &idDir : packs.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir verDir(packs.absoluteFilePath(idDir));
        for (const QString &name : verDir.entryList({"*.nexor"}, QDir::Files)) {
            InstalledRow r;
            r.id          = idDir;
            r.version     = QFileInfo(name).completeBaseName();
            r.packagePath = verDir.absoluteFilePath(name);
            r.projectRoot = QString("%1/%2/%3").arg(extractedDir(), idDir, r.version);
            r.projectFile = QString("%1/%2.pro").arg(r.projectRoot, idDir);
            r.byteSize    = QFileInfo(r.packagePath).size();
            r.title       = sniffTitle(r.packagePath);
            out.append(r);
        }
    }
    std::sort(out.begin(), out.end(),
              [](const InstalledRow &a, const InstalledRow &b){
                  if (a.id != b.id) return a.id < b.id;
                  return a.version < b.version;
              });
    return out;
}

namespace {

void writeText(const QString &path, const QString &content) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(content.toUtf8());
}

QString iso(const QDateTime &dt) {
    return dt.isValid() ? dt.toUTC().toString(Qt::ISODate)
                        : QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

} // namespace

bool PackageCache::writeProjectScaffold(const Package &pkg,
                                        const QString &projectRoot,
                                        QString &outProjectFile,
                                        QString *error) const {
    QDir().mkpath(projectRoot);

    // Materialise activities + each activity's forms.
    for (const PackageEntry &ae : pkg.activities) {
        QString rel  = ae.file.isEmpty()
                         ? QString("activities/%1/%1.aba").arg(ae.id)
                         : ae.file;
        QString abs  = QDir(projectRoot).absoluteFilePath(rel);
        QDir().mkpath(QFileInfo(abs).absolutePath());
        writeText(abs, ae.content);
    }

    // Forms.  Each one's manifest entry already carries its project-relative
    // path, so we honour it.
    for (const PackageEntry &fe : pkg.forms) {
        QString rel  = fe.file.isEmpty()
                         ? QString("activities/%1.frm").arg(fe.id)
                         : fe.file;
        QString abs  = QDir(projectRoot).absoluteFilePath(rel);
        QDir().mkpath(QFileInfo(abs).absolutePath());
        writeText(abs, fe.content);
    }

    for (const PackageEntry &se : pkg.sheets) {
        QString rel  = se.file.isEmpty()
                         ? QString("sheets/%1/%1.sht").arg(se.id)
                         : se.file;
        QString abs  = QDir(projectRoot).absoluteFilePath(rel);
        QDir().mkpath(QFileInfo(abs).absolutePath());
        writeText(abs, se.content);
    }

    for (const PackageEntry &pe : pkg.processes) {
        QString rel  = pe.file.isEmpty()
                         ? QString("processes/%1/%1.bpmn").arg(pe.id)
                         : pe.file;
        QString abs  = QDir(projectRoot).absoluteFilePath(rel);
        QDir().mkpath(QFileInfo(abs).absolutePath());
        writeText(abs, pe.content);
    }

    for (const ResourceEntry &re : pkg.resources) {
        QString abs = QDir(projectRoot).absoluteFilePath(re.file);
        QDir().mkpath(QFileInfo(abs).absolutePath());
        QFile f(abs);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(re.data);
    }

    // Synthesise a .pro that references everything — Studio's Project loader
    // expects this exact shape.  Creating it on disk lets every Studio
    // helper (Project, ProjectTree consumer, FormRunner, ProcessEngine) work
    // unchanged inside Flux.
    QString proPath = QDir(projectRoot).absoluteFilePath(pkg.meta.id + ".pro");
    QFile pf(proPath);
    if (!pf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = "Could not write " + proPath;
        return false;
    }
    QXmlStreamWriter w(&pf);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("NexorProject");
    w.writeAttribute("version", "1");

    w.writeStartElement("Meta");
    w.writeTextElement("Title",       pkg.meta.title);
    w.writeTextElement("Id",          pkg.meta.id);
    w.writeTextElement("Description", QString("Installed from package %1 v%2")
                                          .arg(pkg.meta.id, pkg.meta.version));
    w.writeTextElement("Author",      pkg.meta.builder);
    w.writeTextElement("Created",     iso(pkg.meta.builtAt));
    w.writeEndElement();

    w.writeStartElement("Events"); w.writeEndElement();

    w.writeStartElement("AtomicActivities");
    for (const PackageEntry &ae : pkg.activities) {
        w.writeStartElement("Activity");
        w.writeAttribute("id",   ae.id);
        w.writeAttribute("file", ae.file.isEmpty()
                                    ? QString("activities/%1/%1.aba").arg(ae.id)
                                    : ae.file);
        w.writeEndElement();
    }
    w.writeEndElement();

    w.writeStartElement("ProcessActivities");
    for (const PackageEntry &pe : pkg.processes) {
        w.writeStartElement("Process");
        w.writeAttribute("id",   pe.id);
        w.writeAttribute("file", pe.file.isEmpty()
                                    ? QString("processes/%1/%1.bpmn").arg(pe.id)
                                    : pe.file);
        w.writeEndElement();
    }
    w.writeEndElement();

    w.writeStartElement("Sheets");
    for (const PackageEntry &se : pkg.sheets) {
        w.writeStartElement("Sheet");
        w.writeAttribute("id",   se.id);
        w.writeAttribute("file", se.file.isEmpty()
                                    ? QString("sheets/%1/%1.sht").arg(se.id)
                                    : se.file);
        w.writeEndElement();
    }
    w.writeEndElement();

    w.writeStartElement("Reports");   w.writeEndElement();
    w.writeStartElement("Resources"); w.writeEndElement();

    w.writeEndElement(); // NexorProject
    w.writeEndDocument();
    outProjectFile = proPath;
    return true;
}

bool PackageCache::installFromBytes(const QByteArray &bytes,
                                    InstalledRow &outRow,
                                    QString *error) {
    auto rd = PackageReader::fromBytes(bytes, /*verifyHash*/true);
    if (rd.status != PackageReader::Status::Ok) {
        if (error) *error = "Bad package: " + rd.message;
        return false;
    }
    if (rd.package.meta.id.isEmpty() || rd.package.meta.version.isEmpty()) {
        if (error) *error = "Manifest missing Id or Version.";
        return false;
    }

    QString pkgPath = packagePathFor(rd.package.meta.id, rd.package.meta.version);
    QDir().mkpath(QFileInfo(pkgPath).absolutePath());
    {
        QFile f(pkgPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (error) *error = "Could not write " + pkgPath;
            return false;
        }
        f.write(bytes);
    }

    QString projectRoot = QString("%1/%2/%3").arg(extractedDir(),
                                                  rd.package.meta.id,
                                                  rd.package.meta.version);
    // Wipe any previous extraction of the same version to avoid stale files.
    QDir(projectRoot).removeRecursively();

    QString proPath;
    if (!writeProjectScaffold(rd.package, projectRoot, proPath, error))
        return false;

    outRow.id          = rd.package.meta.id;
    outRow.version     = rd.package.meta.version;
    outRow.title       = rd.package.meta.title;
    outRow.packagePath = pkgPath;
    outRow.projectRoot = projectRoot;
    outRow.projectFile = proPath;
    outRow.byteSize    = bytes.size();
    return true;
}

bool PackageCache::uninstall(const QString &id, const QString &version) {
    QString pkgPath = packagePathFor(id, version);
    QString proot   = QString("%1/%2/%3").arg(extractedDir(), id, version);
    bool any = false;
    if (QFile::exists(pkgPath)) { QFile::remove(pkgPath); any = true; }
    if (QDir(proot).exists())   { QDir(proot).removeRecursively(); any = true; }
    // Also drop the empty parent dirs if they're now empty.
    QDir(packagesDir() + "/" + id).rmdir(packagesDir() + "/" + id);
    QDir(extractedDir() + "/" + id).rmdir(extractedDir() + "/" + id);
    return any;
}

} // namespace nx
