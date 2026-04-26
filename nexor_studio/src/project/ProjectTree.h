// =============================================================================
// ProjectTree — left-dock tree displaying the Project structure with
// right-click context menus for New / Insert actions on group nodes.
// =============================================================================
#ifndef NEXOR_STUDIO_PROJECTTREE_H
#define NEXOR_STUDIO_PROJECTTREE_H

#include <QTreeWidget>
#include <memory>

class Project;
class Activity;

class ProjectTree : public QTreeWidget {
    Q_OBJECT
public:
    enum NodeKind {
        NodeRoot = QTreeWidgetItem::UserType + 1,
        NodeGroupEvents,
        NodeGroupAtomicActivities,
        NodeGroupProcessActivities,
        NodeGroupSheets,
        NodeGroupReports,
        NodeGroupResources,
        NodeActivity,
        NodeForm,
        NodeGenericLeaf
    };

    explicit ProjectTree(QWidget *parent = nullptr);

    void setProject(Project *p);
    void refresh();

signals:
    void requestNewAtomicActivity();
    void requestNewProcessActivity();
    void requestNewSheet();
    void requestInsertAtomicActivities();
    void requestInsertProcessActivities();
    void requestInsertSheets();
    void requestInsertResources();
    void formActivated(const QString &absoluteFormPath);
    void activityActivated(const QString &absoluteActivityPath);

private slots:
    void onContextMenu(const QPoint &p);
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    QTreeWidgetItem *addGroup(QTreeWidgetItem *parent, const QString &label, NodeKind kind);
    void rebuildAtomicActivities(QTreeWidgetItem *group);
    void rebuildList(QTreeWidgetItem *group, const QStringList &items);

    Project *m_project { nullptr };

    QTreeWidgetItem *m_rootItem            { nullptr };
    QTreeWidgetItem *m_groupEvents         { nullptr };
    QTreeWidgetItem *m_groupAtomic         { nullptr };
    QTreeWidgetItem *m_groupProcess        { nullptr };
    QTreeWidgetItem *m_groupSheets         { nullptr };
    QTreeWidgetItem *m_groupReports        { nullptr };
    QTreeWidgetItem *m_groupResources      { nullptr };
};

#endif // NEXOR_STUDIO_PROJECTTREE_H
