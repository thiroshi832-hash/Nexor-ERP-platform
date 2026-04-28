#include "ActivityPicker.h"

#include "../../nexor_studio/src/project/Project.h"
#include "../../nexor_studio/src/project/Activity.h"
#include "../../nexor_studio/src/project/Process.h"
#include "../../nexor_studio/src/runtime/FormRunner.h"
#include "../../nexor_studio/src/runtime/ProcessEngine.h"
#include "../../nexor_studio/src/language/NexorRuntime.h"

#include <QTreeWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QTextCursor>
#include <QMessageBox>
#include <QXmlStreamReader>

namespace nx {

namespace {
constexpr int RoleKind = Qt::UserRole + 1;     // 0=form, 1=activity, 2=process
constexpr int RolePath = Qt::UserRole + 2;     // absolute file path for forms / activities / processes
} // namespace

ActivityPicker::ActivityPicker(const QString &projectFile, QWidget *parent)
    : QDialog(parent), m_projectFile(projectFile) {
    setWindowTitle("Run...");
    resize(720, 540);
    setStyleSheet(R"(
        QDialog { background:#13151b; color:#dce1e7; }
        QLabel  { color:#dce1e7; }
        QLabel#hdr {
            background:#0d0e12; color:#5b8cff;
            padding:10px 14px; border-bottom:1px solid #1e2030;
            font-family:"Segoe UI"; font-size:14px; font-weight:600;
            letter-spacing:1px;
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
        QTreeWidget::item:selected { background:#1e3a5f; color:#ffffff; }
        QPushButton {
            background:#262932; color:#dce1e7;
            border:1px solid #353945; border-radius:4px;
            padding:5px 14px; font-size:12px;
        }
        QPushButton:hover    { background:#2d3140; border-color:#5b8cff; }
        QPushButton:disabled { color:#4b5260; border-color:#2a2d35; }
        QPlainTextEdit#log {
            background:#0d0e12; color:#dce1e7; border:none;
            border-top:1px solid #1e2030;
            font-family:Consolas, "Courier New"; font-size:12px;
        }
    )");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0); root->setSpacing(0);

    auto *hdr = new QLabel("RUN");
    hdr->setObjectName("hdr");
    root->addWidget(hdr);

    m_tree = new QTreeWidget;
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({"Item", "Type"});
    m_tree->setColumnWidth(0, 360);
    m_tree->setIndentation(16);
    m_tree->setUniformRowHeights(true);
    root->addWidget(m_tree, 1);

    auto *btnRow = new QWidget;
    btnRow->setStyleSheet("background:#1b1d23; border-top:1px solid #1e2030;");
    auto *bL = new QHBoxLayout(btnRow);
    bL->setContentsMargins(14, 8, 14, 8); bL->setSpacing(8);
    m_runFormBtn = new QPushButton("▶ Run Form");
    m_runMainBtn = new QPushButton("▶ Run Sub Main");
    m_runProcBtn = new QPushButton("▶ Run Process");
    auto *closeBtn = new QPushButton("Close");
    bL->addWidget(m_runFormBtn);
    bL->addWidget(m_runMainBtn);
    bL->addWidget(m_runProcBtn);
    bL->addStretch();
    bL->addWidget(closeBtn);
    root->addWidget(btnRow);

    m_log = new QPlainTextEdit;
    m_log->setObjectName("log");
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);
    m_log->setFixedHeight(160);
    root->addWidget(m_log);

    connect(m_runFormBtn, &QPushButton::clicked, this, &ActivityPicker::onRunForm);
    connect(m_runMainBtn, &QPushButton::clicked, this, &ActivityPicker::onRunActivityMain);
    connect(m_runProcBtn, &QPushButton::clicked, this, &ActivityPicker::onRunProcess);
    connect(closeBtn,     &QPushButton::clicked, this, &QDialog::accept);

    // Load the Project off the extracted/<id>/<ver>/ tree.
    m_project = std::make_unique<Project>();
    m_project->setFilePath(m_projectFile);
    if (!m_project->load()) {
        QMessageBox::warning(this, "Run...",
            "Could not load project: " + m_projectFile);
        return;
    }
    populateTree();
    appendOutput("Loaded " + m_project->meta().title + "  ("
                 + QFileInfo(m_projectFile).absolutePath() + ")", "#5b8cff");
}

ActivityPicker::~ActivityPicker() = default;

void ActivityPicker::populateTree() {
    if (!m_project) return;

    if (!m_project->atomicActivities().isEmpty()) {
        auto *grp = new QTreeWidgetItem(m_tree, QStringList() << "Activities");
        QFont f = grp->font(0); f.setBold(true);
        grp->setFont(0, f);
        grp->setForeground(0, QBrush(QColor("#8a95a3")));
        for (const auto &act : m_project->atomicActivities()) {
            auto *aItem = new QTreeWidgetItem(grp, QStringList()
                << (act->meta().title.isEmpty() ? act->meta().id : act->meta().title)
                << "Activity");
            aItem->setData(0, RoleKind, 1);
            aItem->setData(0, RolePath, act->filePath());
            aItem->setForeground(0, QBrush(QColor("#a3e635")));
            QString actDir = QFileInfo(act->filePath()).absolutePath();
            for (const QString &form : act->forms()) {
                auto *fItem = new QTreeWidgetItem(aItem, QStringList()
                    << QFileInfo(form).completeBaseName() << "Form");
                fItem->setData(0, RoleKind, 0);
                fItem->setData(0, RolePath,
                    QDir(actDir).absoluteFilePath(form));
                fItem->setForeground(0, QBrush(QColor("#5b8cff")));
            }
            aItem->setExpanded(true);
        }
        grp->setExpanded(true);
    }

    if (!m_project->processActivities().isEmpty()) {
        auto *grp = new QTreeWidgetItem(m_tree, QStringList() << "Processes");
        QFont f = grp->font(0); f.setBold(true);
        grp->setFont(0, f);
        grp->setForeground(0, QBrush(QColor("#8a95a3")));
        for (const auto &prc : m_project->processActivities()) {
            auto *it = new QTreeWidgetItem(grp, QStringList()
                << (prc->meta().title.isEmpty() ? prc->meta().id : prc->meta().title)
                << "Process");
            it->setData(0, RoleKind, 2);
            it->setData(0, RolePath, prc->filePath());
            it->setForeground(0, QBrush(QColor("#c084fc")));
        }
        grp->setExpanded(true);
    }
}

void ActivityPicker::onRunForm() {
    QTreeWidgetItem *it = m_tree->currentItem();
    if (!it || it->data(0, RoleKind).toInt() != 0) {
        appendOutput("Pick a form first.", "#facc15");
        return;
    }
    QString path = it->data(0, RolePath).toString();
    appendOutput("Run form: " + path, "#5b8cff");
    FormRunner::runForm(path, this,
        [this](const QString &line) { appendOutput(line, "#dce1e7"); },
        [this](const QString &err)  { appendOutput("ERROR: " + err, "#ef4444"); },
        m_project.get());
}

void ActivityPicker::onRunActivityMain() {
    QTreeWidgetItem *it = m_tree->currentItem();
    if (!it || it->data(0, RoleKind).toInt() != 1) {
        appendOutput("Pick an activity first.", "#facc15");
        return;
    }
    QString abaPath = it->data(0, RolePath).toString();
    QFile f(abaPath);
    if (!f.open(QIODevice::ReadOnly)) {
        appendOutput("Cannot read " + abaPath, "#ef4444");
        return;
    }
    QXmlStreamReader r(&f);
    QString code;
    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement() && r.name() == "Code")
            code = r.readElementText();
    }
    f.close();

    QString unit = QFileInfo(abaPath).fileName();
    appendOutput("Run activity: " + unit, "#5b8cff");

    NexorRuntime rt;
    rt.setOutput([this](const QString &line){ appendOutput(line, "#dce1e7"); });
    rt.setError ([this](const QString &er) { appendOutput("ERROR: " + er, "#ef4444"); });
    rt.registerProjectSheets(m_project.get());
    if (!rt.compile(code, unit)) {
        appendOutput("Compile error: " + rt.lastError(), "#ef4444");
        return;
    }
    if (!rt.hasSub("Main")) {
        appendOutput("No Sub Main() in " + unit, "#facc15");
        return;
    }
    rt.call("Main");
    appendOutput("Activity finished.", "#22c55e");
}

void ActivityPicker::onRunProcess() {
    QTreeWidgetItem *it = m_tree->currentItem();
    if (!it || it->data(0, RoleKind).toInt() != 2) {
        appendOutput("Pick a process first.", "#facc15");
        return;
    }
    QString prcPath = it->data(0, RolePath).toString();
    appendOutput("Run process: " + prcPath, "#5b8cff");
    bool ok = ProcessEngine::runProcess(prcPath,
        [this](const QString &line) { appendOutput(line, "#dce1e7"); },
        [this](const QString &er)   { appendOutput("ERROR: " + er, "#ef4444"); },
        m_project.get());
    appendOutput(ok ? "Process finished." : "Process aborted.",
                 ok ? "#22c55e" : "#ef4444");
}

void ActivityPicker::appendOutput(const QString &line, const QString &color) {
    QString stamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString html = QString("<span style='color:#6b7280'>%1</span> ").arg(stamp);
    if (!color.isEmpty())
        html += QString("<span style='color:%1'>%2</span>")
                    .arg(color, line.toHtmlEscaped());
    else
        html += line.toHtmlEscaped();
    QTextCursor c(m_log->document());
    c.movePosition(QTextCursor::End);
    c.insertHtml(html + "<br>");
    m_log->setTextCursor(c);
}

} // namespace nx
