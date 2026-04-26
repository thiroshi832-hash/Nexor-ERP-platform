// =============================================================================
// SideBar — slim vertical strip on the far left of the IDE window.
//
// Holds toggle-style buttons that switch the central area between modes:
//   Projects, Edit, Designer, Build, Debug.
// =============================================================================
#ifndef NEXOR_STUDIO_SIDEBAR_H
#define NEXOR_STUDIO_SIDEBAR_H

#include <QWidget>

class QToolButton;
class QButtonGroup;

class SideBar : public QWidget {
    Q_OBJECT
public:
    enum Mode { ModeProjects, ModeEdit, ModeDesigner, ModeBuild, ModeDebug };

    explicit SideBar(QWidget *parent = nullptr);
    void setMode(Mode m);

signals:
    void modeChanged(int mode);

private:
    QToolButton *makeBtn(const QString &text, int id);

    QButtonGroup *m_group;
};

#endif // NEXOR_STUDIO_SIDEBAR_H
