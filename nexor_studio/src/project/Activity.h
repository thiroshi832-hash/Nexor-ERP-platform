// =============================================================================
// Activity — an Atomic Activity (.aba) holding one or more forms.
//
// Disk layout for a project named "MyProj" with activity "DoStuff":
//   MyProj.pro                       — project file (XML)
//   activities/DoStuff/DoStuff.aba   — activity file (XML; contains code + forms list)
//   activities/DoStuff/DoStuff_main.frm — auto-generated main form (XML)
// =============================================================================
#ifndef NEXOR_STUDIO_ACTIVITY_H
#define NEXOR_STUDIO_ACTIVITY_H

#include <QString>
#include <QDateTime>
#include <QStringList>

struct ActivityMeta {
    QString   title;        // human-readable name shown in tree
    QString   id;           // identifier used in code (no spaces)
    QString   description;
    QString   author;
    QDateTime created;
};

class Activity {
public:
    Activity();
    explicit Activity(const ActivityMeta &meta);

    const ActivityMeta &meta() const { return m_meta; }
    void  setMeta(const ActivityMeta &m) { m_meta = m; }

    QString filePath() const { return m_filePath; }
    void    setFilePath(const QString &p) { m_filePath = p; }

    QString code() const { return m_code; }
    void    setCode(const QString &c) { m_code = c; }

    const QStringList &forms() const { return m_forms; }
    void  addForm(const QString &formFile) { m_forms << formFile; }
    void  setForms(const QStringList &f) { m_forms = f; }

    // XML I/O — serialises {meta, code, forms list} as an .aba file.
    bool save() const;
    bool load();

    // Generates an empty form XML file at the given path.
    static bool writeBlankForm(const QString &filePath, const QString &formId);

private:
    ActivityMeta m_meta;
    QString      m_filePath;   // absolute path to the .aba file
    QString      m_code;       // raw source code (custom language)
    QStringList  m_forms;      // .frm filenames relative to the activity directory
};

#endif // NEXOR_STUDIO_ACTIVITY_H
