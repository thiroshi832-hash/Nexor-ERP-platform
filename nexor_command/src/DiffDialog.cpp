#include "DiffDialog.h"

#include <QTreeWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QPushButton>

namespace nx {

namespace {
QColor colourFor(const QString &kind) {
    if (kind == "added")          return QColor("#22c55e");
    if (kind == "removed")        return QColor("#ef4444");
    if (kind == "changed")        return QColor("#facc15");
    if (kind == "type-changed")   return QColor("#fb923c");
    if (kind == "flags-changed")  return QColor("#facc15");
    return QColor("#8a95a3");
}
} // namespace

DiffDialog::DiffDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Package Diff");
    resize(900, 620);
    setStyleSheet(R"(
        QDialog { background:#13151b; color:#dce1e7; }
        QLabel  { color:#dce1e7; font-family:"Segoe UI"; }
        QLabel#hdr {
            background:#0d0e12; color:#fb923c;
            padding:10px 14px; border-bottom:1px solid #1e2030;
            font-size:14px; font-weight:600; letter-spacing:1px;
        }
        QLabel#section {
            background:#0d0e12; color:#8a95a3;
            padding:6px 12px; border-top:1px solid #1e2030;
            border-bottom:1px solid #1e2030;
            font-size:11px; font-weight:600; letter-spacing:2px;
        }
        QTreeWidget {
            background:#1b1d23; color:#dce1e7;
            border:none; gridline-color:#2a3655;
            font-family:"Segoe UI"; font-size:12px;
        }
        QHeaderView::section {
            background:#262932; color:#8a95a3;
            padding:5px 8px; border:none; border-right:1px solid #1e2030;
            font-weight:600; font-size:11px; letter-spacing:1px;
        }
        QPushButton {
            background:#262932; color:#dce1e7;
            border:1px solid #353945; border-radius:4px;
            padding:5px 14px; font-size:12px;
        }
        QPushButton:hover { background:#2d3140; border-color:#5b8cff; }
    )");

    m_header = new QLabel("Package Diff");
    m_header->setObjectName("hdr");

    auto *split = new QSplitter(Qt::Vertical);
    split->setChildrenCollapsible(false);
    split->setHandleWidth(1);
    split->setStyleSheet("QSplitter::handle{background:#1e2030;}");

    auto *topW = new QWidget;
    auto *topL = new QVBoxLayout(topW);
    topL->setContentsMargins(0,0,0,0); topL->setSpacing(0);
    auto *entHdr = new QLabel("ENTRIES"); entHdr->setObjectName("section");
    topL->addWidget(entHdr);
    m_entries = new QTreeWidget;
    m_entries->setColumnCount(5);
    m_entries->setHeaderLabels({"Kind","Section","ID","From","To"});
    m_entries->setRootIsDecorated(false);
    m_entries->setUniformRowHeights(true);
    m_entries->setAlternatingRowColors(false);
    m_entries->setSortingEnabled(true);
    m_entries->header()->setSectionResizeMode(QHeaderView::Interactive);
    m_entries->setColumnWidth(0, 90);
    m_entries->setColumnWidth(1, 110);
    m_entries->setColumnWidth(2, 220);
    m_entries->setColumnWidth(3, 180);
    topL->addWidget(m_entries, 1);
    split->addWidget(topW);

    auto *botW = new QWidget;
    auto *botL = new QVBoxLayout(botW);
    botL->setContentsMargins(0,0,0,0); botL->setSpacing(0);
    auto *shtHdr = new QLabel("SCHEMA MIGRATION (Sheet field changes)");
    shtHdr->setObjectName("section");
    botL->addWidget(shtHdr);
    m_sheets = new QTreeWidget;
    m_sheets->setColumnCount(5);
    m_sheets->setHeaderLabels({"Sheet/Field","Kind","From type","To type","Detail"});
    m_sheets->setUniformRowHeights(true);
    m_sheets->setColumnWidth(0, 220);
    m_sheets->setColumnWidth(1, 110);
    m_sheets->setColumnWidth(2, 100);
    m_sheets->setColumnWidth(3, 100);
    botL->addWidget(m_sheets, 1);
    split->addWidget(botW);

    auto *bb = new QHBoxLayout;
    bb->addStretch();
    auto *closeBtn = new QPushButton("Close");
    closeBtn->setDefault(true);
    bb->addWidget(closeBtn);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0); root->setSpacing(0);
    root->addWidget(m_header);
    root->addWidget(split, 1);
    auto *bbW = new QWidget;
    bbW->setStyleSheet("background:#1b1d23; border-top:1px solid #1e2030;");
    auto *bbL = new QHBoxLayout(bbW);
    bbL->setContentsMargins(14,8,14,8);
    bbL->addStretch();
    bbL->addWidget(closeBtn);
    root->addWidget(bbW);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void DiffDialog::setDiff(const DiffResult &d) {
    m_header->setText(QString("Package Diff   %1 → %2").arg(d.fromVersion, d.toVersion));

    int added = 0, removed = 0, changed = 0;
    for (const auto &e : d.entries) {
        if (e.kind == "added")    ++added;
        if (e.kind == "removed")  ++removed;
        if (e.kind == "changed")  ++changed;
    }
    m_entries->clear();
    for (const auto &e : d.entries) {
        if (e.kind == "unchanged") continue;        // skip noise
        auto *it = new QTreeWidgetItem(QStringList()
            << e.kind << e.section << e.id
            << e.fromHash.left(12) << e.toHash.left(12));
        QColor c = colourFor(e.kind);
        for (int i = 0; i < it->columnCount(); ++i) it->setForeground(i, c);
        m_entries->addTopLevelItem(it);
    }
    m_sheets->clear();
    for (const auto &s : d.sheets) {
        auto *parent = new QTreeWidgetItem(QStringList() << s.sheetId);
        QFont f = parent->font(0); f.setBold(true);
        parent->setFont(0, f);
        m_sheets->addTopLevelItem(parent);
        for (const auto &fc : s.fields) {
            auto *it = new QTreeWidgetItem(parent, QStringList()
                << ("  " + fc.name) << fc.kind << fc.fromType << fc.toType << fc.detail);
            QColor c = colourFor(fc.kind);
            for (int i = 1; i < it->columnCount(); ++i) it->setForeground(i, c);
        }
        parent->setExpanded(true);
    }
    setWindowTitle(QString("Package Diff   +%1 / −%2 / ~%3").arg(added).arg(removed).arg(changed));
}

} // namespace nx
