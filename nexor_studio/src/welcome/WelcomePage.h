// =============================================================================
// WelcomePage — Qt Creator-style start page with Recent Projects, New Project
// and Examples sections.
// =============================================================================
#ifndef NEXOR_STUDIO_WELCOMEPAGE_H
#define NEXOR_STUDIO_WELCOMEPAGE_H

#include <QWidget>

class QListWidget;

class WelcomePage : public QWidget {
    Q_OBJECT
public:
    explicit WelcomePage(QWidget *parent = nullptr);

    void setRecentProjects(const QStringList &paths);

signals:
    void newProjectRequested();
    void openProjectRequested();
    void recentProjectActivated(const QString &path);
    void exampleActivated(const QString &name);

private:
    QListWidget *m_recentList;
    QListWidget *m_exampleList;
};

#endif // NEXOR_STUDIO_WELCOMEPAGE_H
