#include "AuditDialog.h"

#include <QTableWidget>
#include <QLabel>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

namespace nx {

namespace {
QColor colourFor(const QString &eventType) {
    if (eventType == "publish")        return QColor("#5b8cff");
    if (eventType == "deploy")         return QColor("#22c55e");
    if (eventType == "rollback")       return QColor("#ef4444");
    if (eventType == "delete-pending") return QColor("#fb923c");
    return QColor("#dce1e7");
}
} // namespace

AuditDialog::AuditDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Audit Log");
    resize(900, 540);
    setStyleSheet(R"(
        QDialog { background:#13151b; color:#dce1e7; }
        QLabel#hdr {
            background:#0d0e12; color:#fb923c;
            padding:10px 14px; border-bottom:1px solid #1e2030;
            font-family:"Segoe UI"; font-size:14px; font-weight:600;
            letter-spacing:1px;
        }
        QTableWidget {
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

    m_header = new QLabel("Audit Log");
    m_header->setObjectName("hdr");

    m_table = new QTableWidget(0, 6);
    m_table->setHorizontalHeaderLabels(
        QStringList() << "When (UTC)" << "Event" << "Package" << "Version"
                      << "Actor" << "Detail");
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(24);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setColumnWidth(0, 170);
    m_table->setColumnWidth(1, 110);
    m_table->setColumnWidth(2, 130);
    m_table->setColumnWidth(3, 100);
    m_table->setColumnWidth(4, 110);
    m_table->horizontalHeader()->setStretchLastSection(true);

    auto *closeBtn = new QPushButton("Close");
    closeBtn->setDefault(true);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0); root->setSpacing(0);
    root->addWidget(m_header);
    root->addWidget(m_table, 1);
    auto *bbW = new QWidget;
    bbW->setStyleSheet("background:#1b1d23; border-top:1px solid #1e2030;");
    auto *bbL = new QHBoxLayout(bbW);
    bbL->setContentsMargins(14,8,14,8);
    bbL->addStretch();
    bbL->addWidget(closeBtn);
    root->addWidget(bbW);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void AuditDialog::setEvents(const QString &filterPackage,
                            const QVector<AuditRow> &events) {
    m_header->setText(filterPackage.isEmpty()
        ? QString("Audit Log   (%1 events)").arg(events.size())
        : QString("Audit Log — %1   (%2 events)").arg(filterPackage).arg(events.size()));

    m_table->setRowCount(events.size());
    for (int i = 0; i < events.size(); ++i) {
        const auto &e = events.at(i);
        auto put = [&](int col, const QString &text, const QColor &fg = QColor()) {
            auto *it = new QTableWidgetItem(text);
            if (fg.isValid()) it->setForeground(fg);
            m_table->setItem(i, col, it);
        };
        put(0, e.occurredAt);
        put(1, e.eventType, colourFor(e.eventType));
        put(2, e.packageId);
        put(3, e.version);
        put(4, e.actor);
        put(5, e.detail);
    }
}

} // namespace nx
