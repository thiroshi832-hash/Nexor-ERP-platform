// =============================================================================
// Project — a Nexor Studio project (.pro, XML format).
//
// A Project owns six top-level node groups, mirroring the spec:
//   Events            (event hooks — list of names for now)
//   AtomicActivities  (list of Activity objects)
//   ProcessActivities (list of process activity ids)
//   Sheets            (list of sheet ids)
//   Reports           (list of report ids)
//   Resources         (list of resource paths)
// =============================================================================
#ifndef NEXOR_STUDIO_PROJECT_H
#define NEXOR_STUDIO_PROJECT_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QVector>
#include <memory>

#include "Activity.h"
#include "Sheet.h"

struct ProjectMeta {
    QString   title;
    QString   id;
    QString   description;
    QString   author;
    QDateTime created;
};

class Project {
public:
    Project();
    ~Project();

    const ProjectMeta &meta() const { return m_meta; }
    void  setMeta(const ProjectMeta &m) { m_meta = m; }

    QString filePath()  const { return m_filePath;  }
    QString rootDir()   const;        // dir containing the .pro file

    void setFilePath(const QString &p) { m_filePath = p; }

    // Group accessors
    QStringList &events()             { return m_events; }
    QVector<std::shared_ptr<Activity>> &atomicActivities() { return m_atomicActivities; }
    QStringList &processActivities()  { return m_processActivities; }
    QVector<std::shared_ptr<Sheet>> &sheets() { return m_sheets; }
    QStringList &reports()            { return m_reports; }
    QStringList &resources()          { return m_resources; }

    const QStringList &events()             const { return m_events; }
    const QVector<std::shared_ptr<Activity>> &atomicActivities() const { return m_atomicActivities; }
    const QStringList &processActivities()  const { return m_processActivities; }
    const QVector<std::shared_ptr<Sheet>> &sheets() const { return m_sheets; }
    const QStringList &reports()            const { return m_reports; }
    const QStringList &resources()          const { return m_resources; }

    // Higher-level operations
    std::shared_ptr<Activity> createAtomicActivity(const ActivityMeta &meta, QString *errorOut = nullptr);
    std::shared_ptr<Sheet>    createSheet(const SheetMeta &meta, QString *errorOut = nullptr);

    bool save() const;
    bool load();

    // Factory: creates a brand-new project on disk, returns the Project ready-to-use.
    static std::unique_ptr<Project> createOnDisk(const ProjectMeta &meta,
                                                 const QString &rootDir,
                                                 QString *errorOut = nullptr);

private:
    ProjectMeta m_meta;
    QString     m_filePath;

    QStringList                          m_events;
    QVector<std::shared_ptr<Activity>>   m_atomicActivities;
    QStringList                          m_processActivities;
    QVector<std::shared_ptr<Sheet>>      m_sheets;
    QStringList                          m_reports;
    QStringList                          m_resources;
};

#endif // NEXOR_STUDIO_PROJECT_H
