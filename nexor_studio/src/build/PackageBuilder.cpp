#include "PackageBuilder.h"
#include "project/Project.h"
#include "project/Activity.h"
#include "project/Sheet.h"
#include "project/Process.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QXmlStreamWriter>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QDateTime>

namespace nx {

namespace {

// Reads a file as UTF-8 text, returns empty string on failure.
QString readTextFile(const QString &path, bool *okOut = nullptr) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (okOut) *okOut = false;
        return {};
    }
    if (okOut) *okOut = true;
    return QString::fromUtf8(f.readAll());
}

// Walk the activity dir for any .frm files referenced by the activity.
QString relativeProjectPath(const QString &absPath, const QString &projectRoot) {
    return QDir(projectRoot).relativeFilePath(absPath);
}

// SHA-256 over a canonical content stream — id|file|content per entry, in
// declaration order — followed by the resource bytes.  The same input always
// produces the same hash; reordering entries (or mutating any byte) changes
// it.
QString computeHash(const Package &p) {
    QCryptographicHash h(QCryptographicHash::Sha256);
    auto absorb = [&](const QVector<PackageEntry> &xs) {
        for (const PackageEntry &e : xs) {
            h.addData(e.id.toUtf8());     h.addData("\x1f", 1);
            h.addData(e.file.toUtf8());   h.addData("\x1f", 1);
            h.addData(e.content.toUtf8());h.addData("\x1e", 1);
        }
    };
    absorb(p.activities);
    absorb(p.forms);
    absorb(p.sheets);
    absorb(p.processes);
    for (const ResourceEntry &r : p.resources) {
        h.addData(r.file.toUtf8());  h.addData("\x1f", 1);
        h.addData(r.data);           h.addData("\x1e", 1);
    }
    return QString::fromLatin1(h.result().toHex());
}

// Signature is HMAC-SHA256 over (id|version|builtAt|hash) — that is, over the
// pieces of the manifest that pin a package's identity.  Tampering with any
// artifact changes `hash`, which changes the signature, so signing the hash
// transitively signs the contents.
QString computeSignature(const Package &p, const QByteArray &key) {
    if (key.isEmpty()) return {};
    QMessageAuthenticationCode mac(QCryptographicHash::Sha256);
    mac.setKey(key);
    mac.addData(p.meta.id.toUtf8());        mac.addData("\x1f", 1);
    mac.addData(p.meta.version.toUtf8());   mac.addData("\x1f", 1);
    mac.addData(p.meta.builtAt.toUTC().toString(Qt::ISODate).toUtf8());
    mac.addData("\x1f", 1);
    mac.addData(p.meta.hash.toUtf8());
    return QString::fromLatin1(mac.result().toHex());
}

// Writes a section of homogeneous entries: <Forms><Form …><![CDATA[…]]>…
void writeEntrySection(QXmlStreamWriter &w,
                       const QString &sectionName,
                       const QString &entryName,
                       const QVector<PackageEntry> &entries) {
    w.writeStartElement(sectionName);
    for (const PackageEntry &e : entries) {
        w.writeStartElement(entryName);
        if (!e.id.isEmpty())   w.writeAttribute("id",   e.id);
        if (!e.file.isEmpty()) w.writeAttribute("file", e.file);
        // CDATA preserves the original bytes verbatim (the artifact is
        // already valid XML, so keeping it opaque to the outer document is
        // both simpler and safer than re-encoding).
        w.writeCDATA(e.content);
        w.writeEndElement();
    }
    w.writeEndElement();
}

} // namespace

bool PackageBuilder::collect(const Project &project, Package &out, QString *error) {
    out = Package{};
    QString root = project.rootDir();
    if (root.isEmpty()) {
        if (error) *error = "Project has no root directory (save it first).";
        return false;
    }

    // Activities — emit the .aba itself, then every form it references.
    for (const auto &act : project.atomicActivities()) {
        const QString abaAbs = act->filePath();
        const QString actDir = QFileInfo(abaAbs).absolutePath();

        bool ok = false;
        QString aba = readTextFile(abaAbs, &ok);
        if (!ok) {
            if (error) *error = "Could not read activity: " + abaAbs;
            return false;
        }
        PackageEntry pe;
        pe.id      = act->meta().id;
        pe.file    = relativeProjectPath(abaAbs, root);
        pe.content = aba;
        out.activities.append(pe);

        for (const QString &formFile : act->forms()) {
            QString frmAbs = QDir(actDir).absoluteFilePath(formFile);
            bool fok = false;
            QString frm = readTextFile(frmAbs, &fok);
            if (!fok) continue;        // missing form — skip but don't abort
            PackageEntry fe;
            fe.id      = QFileInfo(formFile).completeBaseName();
            fe.file    = relativeProjectPath(frmAbs, root);
            fe.content = frm;
            out.forms.append(fe);
        }
    }

    // Sheets
    for (const auto &sht : project.sheets()) {
        bool ok = false;
        QString xml = readTextFile(sht->filePath(), &ok);
        if (!ok) {
            if (error) *error = "Could not read sheet: " + sht->filePath();
            return false;
        }
        PackageEntry pe;
        pe.id      = sht->meta().id;
        pe.file    = relativeProjectPath(sht->filePath(), root);
        pe.content = xml;
        out.sheets.append(pe);
    }

    // Processes
    for (const auto &prc : project.processActivities()) {
        bool ok = false;
        QString xml = readTextFile(prc->filePath(), &ok);
        if (!ok) {
            if (error) *error = "Could not read process: " + prc->filePath();
            return false;
        }
        PackageEntry pe;
        pe.id      = prc->meta().id;
        pe.file    = relativeProjectPath(prc->filePath(), root);
        pe.content = xml;
        out.processes.append(pe);
    }

    // Resources — read each path the project lists, fall back to project-rel.
    for (const QString &res : project.resources()) {
        QString abs = QDir::isAbsolutePath(res) ? res
                                                : QDir(root).absoluteFilePath(res);
        QFile f(abs);
        if (!f.open(QIODevice::ReadOnly)) continue;
        ResourceEntry re;
        re.file = relativeProjectPath(abs, root);
        re.data = f.readAll();
        out.resources.append(re);
    }

    out.meta.title = project.meta().title;
    out.meta.id    = project.meta().id;
    return true;
}

QByteArray PackageBuilder::serialise(Package &package, const QByteArray &signingKey) {
    package.meta.hash      = computeHash(package);
    package.meta.signature = computeSignature(package, signingKey);

    QByteArray buf;
    QXmlStreamWriter w(&buf);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("NexorPackage");
    w.writeAttribute("version", "1");

    // Manifest
    w.writeStartElement("Meta");
    w.writeTextElement("Title",   package.meta.title);
    w.writeTextElement("Id",      package.meta.id);
    w.writeTextElement("Version", package.meta.version);
    w.writeTextElement("BuiltAt", package.meta.builtAt.toUTC().toString(Qt::ISODate));
    w.writeTextElement("Builder", package.meta.builder);
    w.writeStartElement("Hash");
    w.writeAttribute("algo", package.meta.hashAlgo);
    w.writeCharacters(package.meta.hash);
    w.writeEndElement();
    if (!package.meta.signature.isEmpty()) {
        w.writeStartElement("Signature");
        w.writeAttribute("algo", package.meta.sigAlgo);
        w.writeCharacters(package.meta.signature);
        w.writeEndElement();
    }
    w.writeEndElement(); // Meta

    writeEntrySection(w, "Activities", "Activity", package.activities);
    writeEntrySection(w, "Forms",      "Form",     package.forms);
    writeEntrySection(w, "Sheets",     "Sheet",    package.sheets);
    writeEntrySection(w, "Processes",  "Process",  package.processes);

    // Resources are base64-encoded so any binary survives XML.
    w.writeStartElement("Resources");
    for (const ResourceEntry &r : package.resources) {
        w.writeStartElement("Resource");
        w.writeAttribute("file",    r.file);
        w.writeAttribute("encoding","base64");
        w.writeCharacters(QString::fromLatin1(r.data.toBase64()));
        w.writeEndElement();
    }
    w.writeEndElement();

    w.writeEndElement(); // NexorPackage
    w.writeEndDocument();
    return buf;
}

PackageBuilder::Result
PackageBuilder::buildAndWrite(const Project &project,
                              const QString &version,
                              const QByteArray &signingKey) {
    Result r;

    Package pkg;
    QString err;
    if (!collect(project, pkg, &err)) { r.error = err; return r; }

    pkg.meta.version  = version.trimmed().isEmpty() ? "0.0.0" : version.trimmed();
    pkg.meta.builtAt  = QDateTime::currentDateTimeUtc();
    pkg.meta.builder  = "Nexor Studio 0.1.0";

    QByteArray bytes = serialise(pkg, signingKey);

    QString root = project.rootDir();
    QString distDir  = root + "/dist";
    if (!QDir().mkpath(distDir)) {
        r.error = "Could not create output directory: " + distDir;
        return r;
    }
    QString fileName = QString("%1-%2.nexor")
        .arg(pkg.meta.id.isEmpty() ? "package" : pkg.meta.id, pkg.meta.version);
    QString outPath  = QDir(distDir).absoluteFilePath(fileName);

    QFile f(outPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        r.error = "Could not write package: " + outPath;
        return r;
    }
    f.write(bytes);
    f.close();

    r.ok          = true;
    r.outputPath  = outPath;
    r.meta        = pkg.meta;
    return r;
}

} // namespace nx
