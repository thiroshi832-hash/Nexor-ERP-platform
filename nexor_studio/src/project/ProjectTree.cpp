#include "ProjectTree.h"
#include "Project.h"
#include "Activity.h"
#include "Sheet.h"

#include <QHeaderView>
#include <QMenu>
#include <QFileInfo>
#include <QDir>

ProjectTree::ProjectTree(QWidget *parent) : QTreeWidget(parent) {
    setHeaderHidden(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setUniformRowHeights(true);
    setIndentation(14);
    setStyleSheet(R"(
        QTreeWidget { background:#1b1d23; color:#dce1e7; border:none;
                      font-family:"Segoe UI"; font-size:13px; }
        QTreeWidget::item { padding:3px 2px; }
        QTreeWidget::item:selected { background:#1e3a5f; }
        QTreeWidget::branch { background:#1b1d23; }
    )");

    connect(this, &QTreeWidget::customContextMenuRequested,
            this, &ProjectTree::onContextMenu);
    connect(this, &QTreeWidget::itemDoubleClicked,
            this, &ProjectTree::onItemDoubleClicked);
}

void ProjectTree::setProject(Project *p) {
    m_project = p;
    refresh();
}

QTreeWidgetItem *ProjectTree::addGroup(QTreeWidgetItem *parent,
                                       const QString &label, NodeKind kind) {
    auto *it = new QTreeWidgetItem(parent, kind);
    it->setText(0, label);
    QFont f = it->font(0); f.setWeight(QFont::DemiBold);
    it->setFont(0, f);
    it->setForeground(0, QBrush(QColor("#8a95a3")));
    return it;
}

void ProjectTree::refresh() {
    clear();
    m_rootItem = nullptr;
    m_groupEvents = m_groupAtomic = m_groupProcess =
        m_groupSheets = m_groupReports = m_groupResources = nullptr;
    if (!m_project) return;

    m_rootItem = new QTreeWidgetItem(this, NodeRoot);
    QString rootLabel = m_project->meta().title.isEmpty()
        ? "Project"
        : m_project->meta().title;
    m_rootItem->setText(0, rootLabel);
    QFont f = m_rootItem->font(0); f.setBold(true);
    m_rootItem->setFont(0, f);
    m_rootItem->setForeground(0, QBrush(QColor("#5b8cff")));

    m_groupEvents    = addGroup(m_rootItem, "Events",             NodeGroupEvents);
    m_groupAtomic    = addGroup(m_rootItem, "Atomic Activities",  NodeGroupAtomicActivities);
    m_groupProcess   = addGroup(m_rootItem, "Process Activities", NodeGroupProcessActivities);
    m_groupSheets    = addGroup(m_rootItem, "Sheets",             NodeGroupSheets);
    m_groupReports   = addGroup(m_rootItem, "Reports",            NodeGroupReports);
    m_groupResources = addGroup(m_rootItem, "Resources",          NodeGroupResources);

    rebuildList(m_groupEvents,    m_project->events());
    rebuildAtomicActivities(m_groupAtomic);
    rebuildList(m_groupProcess,   m_project->processActivities());
    rebuildSheets(m_groupSheets);
    rebuildList(m_groupReports,   m_project->reports());
    rebuildList(m_groupResources, m_project->resources());

    expandItem(m_rootItem);
    expandItem(m_groupAtomic);
}

void ProjectTree::rebuildList(QTreeWidgetItem *group, const QStringList &items) {
    if (!group) return;
    for (const QString &name : items) {
        auto *it = new QTreeWidgetItem(group, NodeGenericLeaf);
        it->setText(0, name);
    }
}

void ProjectTree::rebuildAtomicActivities(QTreeWidgetItem *group) {
    if (!group || !m_project) return;
    for (const auto &act : m_project->atomicActivities()) {
        auto *aItem = new QTreeWidgetItem(group, NodeActivity);
        aItem->setText(0, act->meta().title.isEmpty() ? act->meta().id : act->meta().title);
        aItem->setData(0, Qt::UserRole, act->filePath());

        // Forms under this activity
        const QString actDir = QFileInfo(act->filePath()).absolutePath();
        for (const QString &form : act->forms()) {
            auto *fItem = new QTreeWidgetItem(aItem, NodeForm);
            fItem->setText(0, form);
            fItem->setForeground(0, QBrush(QColor("#a3e635")));
            fItem->setData(0, Qt::UserRole, QDir(actDir).absoluteFilePath(form));
        }
        aItem->setExpanded(true);
    }
}

void ProjectTree::onContextMenu(const QPoint &p) {
    QTreeWidgetItem *item = itemAt(p);
    if (!item) return;
    NodeKind kind = static_cast<NodeKind>(item->type());

    QMenu menu(this);
    switch (kind) {
    case NodeGroupAtomicActivities:
        menu.addAction("New Atomic Activity...",     this, &ProjectTree::requestNewAtomicActivity);
        menu.addAction("Insert Atomic Activities...", this, &ProjectTree::requestInsertAtomicActivities);
        break;
    case NodeGroupProcessActivities:
        menu.addAction("New Process Activity...",     this, &ProjectTree::requestNewProcessActivity);
        menu.addAction("Insert Process Activities...", this, &ProjectTree::requestInsertProcessActivities);
        break;
    case NodeGroupSheets:
        menu.addAction("New Sheet...",     this, &ProjectTree::requestNewSheet);
        menu.addAction("Insert Sheets...", this, &ProjectTree::requestInsertSheets);
        break;
    case NodeGroupResources:
        menu.addAction("Insert Resources...", this, &ProjectTree::requestInsertResources);
        break;
    default:
        return; // nothing for other kinds (yet)
    }
    menu.exec(viewport()->mapToGlobal(p));
}

void ProjectTree::onItemDoubleClicked(QTreeWidgetItem *item, int /*column*/) {
    if (!item) return;
    NodeKind kind = static_cast<NodeKind>(item->type());
    if (kind == NodeForm) {
        emit formActivated(item->data(0, Qt::UserRole).toString());
    } else if (kind == NodeActivity) {
        emit activityActivated(item->data(0, Qt::UserRole).toString());
    } else if (kind == NodeSheet) {
        emit sheetActivated(item->data(0, Qt::UserRole).toString());
    }
}

void ProjectTree::rebuildSheets(QTreeWidgetItem *group) {
    if (!group || !m_project) return;
    for (const auto &sht : m_project->sheets()) {
        auto *it = new QTreeWidgetItem(group, NodeSheet);
        it->setText(0, sht->meta().title.isEmpty() ? sht->meta().id : sht->meta().title);
        it->setForeground(0, QBrush(QColor("#fbbf24")));   // amber for entities
        it->setData(0, Qt::UserRole, sht->filePath());
    }
}
