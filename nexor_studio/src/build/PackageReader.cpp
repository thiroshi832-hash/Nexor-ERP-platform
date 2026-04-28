#include "PackageReader.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QCryptographicHash>

namespace nx {

namespace {

// Mirror of computeHash in PackageBuilder.cpp.  Kept private to that file,
// re-implemented here to avoid making the canonical-hash function part of
// the public API.  Any drift between the two breaks round-trip; callers
// catch it via Status::HashMismatch.
QString recomputeHash(const Package &p) {
    QCryptographicHash h(QCryptographicHash::Sha256);
    auto absorb = [&](const QVector<PackageEntry> &xs) {
        for (const PackageEntry &e : xs) {
            h.addData(e.id.toUtf8());      h.addData("\x1f", 1);
            h.addData(e.file.toUtf8());    h.addData("\x1f", 1);
            h.addData(e.content.toUtf8()); h.addData("\x1e", 1);
        }
    };
    absorb(p.activities);
    absorb(p.forms);
    absorb(p.sheets);
    absorb(p.processes);
    for (const ResourceEntry &r : p.resources) {
        h.addData(r.file.toUtf8());        h.addData("\x1f", 1);
        h.addData(r.data);                 h.addData("\x1e", 1);
    }
    return QString::fromLatin1(h.result().toHex());
}

void readEntrySection(QXmlStreamReader &r,
                      const QString &endTag,
                      const QString &entryTag,
                      QVector<PackageEntry> &out) {
    while (!r.atEnd()) {
        r.readNext();
        if (r.isEndElement() && r.name() == endTag) return;
        if (r.isStartElement() && r.name() == entryTag) {
            PackageEntry e;
            e.id   = r.attributes().value("id").toString();
            e.file = r.attributes().value("file").toString();
            // readElementText with IncludeChildElements consumes the CDATA
            // section as plain text (CDATA is just escaping; the bytes inside
            // come back verbatim).
            e.content = r.readElementText(QXmlStreamReader::IncludeChildElements);
            out.append(e);
        }
    }
}

void readResources(QXmlStreamReader &r, QVector<ResourceEntry> &out) {
    while (!r.atEnd()) {
        r.readNext();
        if (r.isEndElement() && r.name() == "Resources") return;
        if (r.isStartElement() && r.name() == "Resource") {
            ResourceEntry e;
            e.file = r.attributes().value("file").toString();
            QString enc = r.attributes().value("encoding").toString();
            QString text = r.readElementText();
            e.data = (enc == "base64")
                        ? QByteArray::fromBase64(text.toLatin1())
                        : text.toUtf8();
            out.append(e);
        }
    }
}

} // namespace

PackageReader::Result PackageReader::fromBytes(const QByteArray &bytes, bool verifyHash) {
    Result res;
    QXmlStreamReader r(bytes);
    bool sawRoot = false;

    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement()) continue;
        const auto name = r.name();

        if (name == "NexorPackage") {
            sawRoot = true;
            QString ver = r.attributes().value("version").toString();
            if (ver != "1") {
                res.status  = Status::VersionMismatch;
                res.message = "Unsupported package version: " + ver;
                return res;
            }
        } else if (name == "Title")   res.package.meta.title    = r.readElementText();
        else  if (name == "Id")       res.package.meta.id       = r.readElementText();
        else  if (name == "Version")  res.package.meta.version  = r.readElementText();
        else  if (name == "BuiltAt")  res.package.meta.builtAt  = QDateTime::fromString(r.readElementText(), Qt::ISODate);
        else  if (name == "Builder")  res.package.meta.builder  = r.readElementText();
        else  if (name == "Hash")  {
            res.package.meta.hashAlgo = r.attributes().value("algo").toString();
            if (res.package.meta.hashAlgo.isEmpty()) res.package.meta.hashAlgo = "sha256";
            res.package.meta.hash     = r.readElementText();
        }
        else  if (name == "Activities") readEntrySection(r, "Activities", "Activity", res.package.activities);
        else  if (name == "Forms")      readEntrySection(r, "Forms",      "Form",     res.package.forms);
        else  if (name == "Sheets")     readEntrySection(r, "Sheets",     "Sheet",    res.package.sheets);
        else  if (name == "Processes")  readEntrySection(r, "Processes",  "Process",  res.package.processes);
        else  if (name == "Resources")  readResources(r, res.package.resources);
    }

    if (r.hasError() || !sawRoot) {
        res.status  = Status::ParseError;
        res.message = sawRoot ? r.errorString() : "Not a NexorPackage document.";
        return res;
    }

    if (verifyHash && !res.package.meta.hash.isEmpty()) {
        QString actual = recomputeHash(res.package);
        if (actual.compare(res.package.meta.hash, Qt::CaseInsensitive) != 0) {
            res.status  = Status::HashMismatch;
            res.message = QString("Hash mismatch: manifest=%1 actual=%2")
                              .arg(res.package.meta.hash, actual);
            return res;
        }
    }
    return res;
}

PackageReader::Result PackageReader::fromFile(const QString &path, bool verifyHash) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        Result r;
        r.status  = Status::FileNotFound;
        r.message = "Could not open: " + path;
        return r;
    }
    return fromBytes(f.readAll(), verifyHash);
}

} // namespace nx
