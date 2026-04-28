#include "FluxWindow.h"
#include "ActivityPicker.h"

#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QTextCursor>

namespace nx {

namespace {
QColor statusColour(const QString &s) {
    if (s == "live")        return QColor("#22c55e");
    if (s == "pending")     return QColor("#facc15");
    if (s == "rolled_back") return QColor("#ef4444");
    if (s == "installed")   return QColor("#5b8cff");
    return QColor("#8a95a3");
}
} // namespace

FluxWindow::FluxWindow(QWidget *parent) : QMainWindow(parent),
        m_client(new CoreClient(this)) {
    setWindowTitle("Nexor Flux");
    resize(1100, 700);

    QString cacheRoot = QDir(QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation)).absoluteFilePath("Nexor/Flux");
    m_cache = std::make_unique<PackageCache>(cacheRoot);
    QString err;
    m_cache->prepare(&err);

    setupUi();
    buildMenus();

    connect(m_client, &CoreClient::healthReceived,    this, &FluxWindow::onHealthReceived);
    connect(m_client, &CoreClient::catalogReceived,   this, &FluxWindow::onCatalogReceived);
    connect(m_client, &CoreClient::downloadFinished,  this, &FluxWindow::onDownloadFinished);

    loadSettings();
    log("Nexor Flux started.   Cache: " + cacheRoot, "#5b8cff");
    m_installed = m_cache->list();
    rebuildTable();
    m_client->checkHealth();
    m_client->listCatalog();
}

FluxWindow::~FluxWindow() = default;

void FluxWindow::setupUi() {
    setStyleSheet(R"(
        QMainWindow      { background:#13151b; }
        QMenuBar         { background:#0d0e12; color:#dce1e7;
                           border-bottom:1px solid #1e2030; }
        QMenuBar::item   { padding:5px 12px; }
        QMenuBar::item:selected { background:#1e3a5f; }
        QMenu            { background:#1b1d23; color:#dce1e7;
                           border:1px solid #2a3655; }
        QMenu::item:selected { background:#1e3a5f; }
        QStatusBar       { background:#0d0e12; color:#8a95a3;
                           border-top:1px solid #1e2030; padding:0 8px; }
        QLabel           { color:#dce1e7; }
        QLabel#title {
            background:#0d0e12; color:#5b8cff;
            padding:10px 14px; border-bottom:1px solid #1e2030;
            font-family:"Segoe UI"; font-size:14px; font-weight:600;
            letter-spacing:1px;
        }
        QLabel#health    { color:#8a95a3; font-size:12px; padding-right:14px; }
        QPushButton {
            background:#262932; color:#dce1e7;
            border:1px solid #353945; border-radius:4px;
            padding:5px 14px; font-size:12px;
        }
        QPushButton:hover            { background:#2d3140; border-color:#5b8cff; }
        QPushButton:disabled         { color:#4b5260; border-color:#2a2d35; }
        QPushButton#runBtn           { background:#1e3a5f; }
        QPushButton#runBtn:hover     { background:#26477a; }
        QTableWidget {
            background:#1b1d23; color:#dce1e7;
            gridline-color:#2a3655;
            border:none; outline:0;
            font-family:"Segoe UI"; font-size:12px;
        }
        QHeaderView::section {
            background:#262932; color:#8a95a3;
            padding:5px 8px; border:none; border-right:1px solid #1e2030;
            font-weight:600; font-size:11px; letter-spacing:1px;
        }
        QTableWidget::item:selected { background:#1e3a5f; color:#ffffff; }
        QPlainTextEdit#log {
            background:#0d0e12; color:#dce1e7;
            border:none; border-top:1px solid #1e2030;
            font-family:Consolas, "Courier New"; font-size:12px;
        }
    )");

    auto *root = new QWidget;
    auto *col  = new QVBoxLayout(root);
    col->setContentsMargins(0,0,0,0); col->setSpacing(0);

    auto *titleRow = new QWidget;
    auto *trL = new QHBoxLayout(titleRow);
    trL->setContentsMargins(0,0,0,0); trL->setSpacing(0);
    m_title = new QLabel("NEXOR FLUX");
    m_title->setObjectName("title");
    trL->addWidget(m_title, 1);
    m_health = new QLabel("● disconnected");
    m_health->setObjectName("health");
    trL->addWidget(m_health);
    col->addWidget(titleRow);

    m_table = new QTableWidget(0, 6);
    m_table->setHorizontalHeaderLabels(
        QStringList() << "Source" << "Project" << "Version" << "Status"
                      << "Title" << "Size (B)");
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(26);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setColumnWidth(0, 100);
    m_table->setColumnWidth(1, 200);
    m_table->setColumnWidth(2, 100);
    m_table->setColumnWidth(3, 110);
    m_table->setColumnWidth(4, 240);
    m_table->horizontalHeader()->setStretchLastSection(true);
    col->addWidget(m_table, 1);

    auto *btnRow = new QWidget;
    btnRow->setStyleSheet("background:#1b1d23; border-top:1px solid #1e2030;");
    auto *bL = new QHBoxLayout(btnRow);
    bL->setContentsMargins(14, 8, 14, 8); bL->setSpacing(8);
    m_runBtn       = new QPushButton("▶ Run");        m_runBtn->setObjectName("runBtn");
    m_installBtn   = new QPushButton("⤓ Install");
    m_uninstallBtn = new QPushButton("× Uninstall");
    m_refreshBtn   = new QPushButton("Refresh");
    bL->addWidget(m_runBtn);
    bL->addWidget(m_installBtn);
    bL->addWidget(m_uninstallBtn);
    bL->addStretch();
    bL->addWidget(m_refreshBtn);
    col->addWidget(btnRow);

    m_log = new QPlainTextEdit;
    m_log->setObjectName("log");
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);
    m_log->setFixedHeight(150);
    col->addWidget(m_log);

    setCentralWidget(root);
    statusBar()->showMessage("Ready");

    connect(m_refreshBtn,   &QPushButton::clicked, this, &FluxWindow::onRefresh);
    connect(m_runBtn,       &QPushButton::clicked, this, &FluxWindow::onRun);
    connect(m_installBtn,   &QPushButton::clicked, this, &FluxWindow::onInstall);
    connect(m_uninstallBtn, &QPushButton::clicked, this, &FluxWindow::onUninstall);
    connect(m_table, &QTableWidget::itemDoubleClicked, this,
        [this](QTableWidgetItem*){
            // Double-click runs an installed row, installs an available one.
            int row = m_table->currentRow();
            if (row < 0) return;
            QTableWidgetItem *it = m_table->item(row, 0);
            if (!it) return;
            int src = it->data(RoleSrc).toInt();
            if (src == 0) onRun(); else onInstall();
        });
}

void FluxWindow::buildMenus() {
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Settings...", this, &FluxWindow::onSettings,
                        QKeySequence("Ctrl+,"));
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence("Ctrl+Q"));

    auto *runMenu = menuBar()->addMenu("&Run");
    runMenu->addAction("&Refresh catalog",   this, &FluxWindow::onRefresh,   QKeySequence("F5"));
    runMenu->addAction("&Install selected",  this, &FluxWindow::onInstall);
    runMenu->addAction("&Run selected",      this, &FluxWindow::onRun,       QKeySequence("F6"));
    runMenu->addAction("&Uninstall selected",this, &FluxWindow::onUninstall);

    auto *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About", this, [this]{
        QMessageBox::about(this, "About Nexor Flux",
            "<h2>Nexor Flux</h2>"
            "<p>Version 0.1.0 — desktop end-user runtime.</p>"
            "<p>Catalogs and runs published <code>.nexor</code> packages.</p>");
    });
}

void FluxWindow::loadSettings() {
    QSettings s;
    QString url = s.value("Core/Url", "http://localhost:7421").toString();
    m_client->setBaseUrl(url);
    m_title->setText("NEXOR FLUX   —   Core: " + url);
}

void FluxWindow::onSettings() {
    QSettings s;
    bool ok = false;
    QString url = QInputDialog::getText(this, "Settings",
        "Nexor Core URL:", QLineEdit::Normal,
        s.value("Core/Url", "http://localhost:7421").toString(), &ok).trimmed();
    if (!ok || url.isEmpty()) return;
    s.setValue("Core/Url", url);
    loadSettings();
    log("Connection settings saved.", "#8a95a3");
    onRefresh();
}

void FluxWindow::onRefresh() {
    log("→ catalog", "#5b8cff");
    m_installed = m_cache->list();
    m_client->checkHealth();
    m_client->listCatalog();
}

void FluxWindow::onCatalogReceived(const QVector<CatalogRow> &rows) {
    m_catalog = rows;
    log(QString("← %1 row(s)").arg(rows.size()), "#22c55e");
    rebuildTable();
}

void FluxWindow::onHealthReceived(bool ok, const QString &info) {
    if (ok) {
        m_health->setText("● live");
        m_health->setStyleSheet("color:#22c55e; font-size:12px; padding-right:14px;");
    } else {
        m_health->setText("● unreachable");
        m_health->setStyleSheet("color:#ef4444; font-size:12px; padding-right:14px;");
        log("health: " + info, "#ef4444");
    }
}

void FluxWindow::rebuildTable() {
    // Build a unified row list: every installed package + every available
    // catalog row.  An (id, version) appears at most once; preference goes
    // to the installed copy because the user cares about what's local.
    struct Row {
        bool installed;
        QString id, version, status, title;
        qint64  size;
    };
    QVector<Row> rows;
    auto seen = [&](const QString &id, const QString &ver) {
        for (const auto &r : rows) if (r.id == id && r.version == ver) return true;
        return false;
    };
    for (const auto &r : m_installed)
        rows.append({true,  r.id, r.version, "installed", r.title, r.byteSize});
    for (const auto &r : m_catalog) {
        if (seen(r.id, r.version)) continue;
        rows.append({false, r.id, r.version, r.status, r.title, r.byteSize});
    }
    std::sort(rows.begin(), rows.end(), [](const Row &a, const Row &b){
        if (a.installed != b.installed) return a.installed && !b.installed;
        if (a.id != b.id) return a.id < b.id;
        return a.version < b.version;
    });

    m_table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const auto &r = rows.at(i);
        auto put = [&](int col, const QString &text, const QColor &fg = QColor()) {
            auto *it = new QTableWidgetItem(text);
            if (fg.isValid()) it->setForeground(fg);
            m_table->setItem(i, col, it);
        };
        put(0, r.installed ? "INSTALLED" : "AVAILABLE",
              r.installed ? QColor("#5b8cff") : QColor("#8a95a3"));
        put(1, r.id);
        put(2, r.version);
        put(3, r.status, statusColour(r.status));
        put(4, r.title);
        put(5, QString::number(r.size));
        // Stash routing data on column 0.
        m_table->item(i, 0)->setData(RoleSrc, r.installed ? 0 : 1);
        m_table->item(i, 0)->setData(RoleId, r.id);
        m_table->item(i, 0)->setData(RoleVer, r.version);
    }
}

void FluxWindow::onInstall() {
    int row = m_table->currentRow();
    if (row < 0) { log("Pick a row first.", "#facc15"); return; }
    QTableWidgetItem *it = m_table->item(row, 0);
    QString id  = it->data(RoleId).toString();
    QString ver = it->data(RoleVer).toString();
    QString dest = m_cache->packagePathFor(id, ver);
    log(QString("→ install %1 v%2").arg(id, ver), "#5b8cff");
    m_client->downloadPackage(id, ver, dest);
}

void FluxWindow::onDownloadFinished(const QString &id, const QString &version,
                                    bool ok, const QString &localPath,
                                    const QString &message) {
    if (!ok) { log("✗ " + message, "#ef4444"); return; }
    log("← " + message, "#22c55e");

    // Re-read the bytes we just wrote and explode them into a Project tree.
    QFile f(localPath);
    if (!f.open(QIODevice::ReadOnly)) {
        log("Cannot re-open " + localPath, "#ef4444");
        return;
    }
    QByteArray bytes = f.readAll();
    f.close();
    InstalledRow ins;
    QString err;
    if (!m_cache->installFromBytes(bytes, ins, &err)) {
        log("Extract failed: " + err, "#ef4444");
        return;
    }
    log(QString("Extracted %1 v%2 → %3").arg(id, version, ins.projectRoot),
        "#a3e635");
    m_installed = m_cache->list();
    rebuildTable();
}

void FluxWindow::onUninstall() {
    int row = m_table->currentRow();
    if (row < 0) return;
    QTableWidgetItem *it = m_table->item(row, 0);
    if (it->data(RoleSrc).toInt() != 0) {
        log("Pick an installed row.", "#facc15");
        return;
    }
    QString id  = it->data(RoleId).toString();
    QString ver = it->data(RoleVer).toString();
    if (QMessageBox::question(this, "Uninstall",
            QString("Remove %1 v%2 from local cache?").arg(id, ver))
        != QMessageBox::Yes) return;
    if (m_cache->uninstall(id, ver))
        log(QString("Uninstalled %1 v%2.").arg(id, ver), "#a3e635");
    m_installed = m_cache->list();
    rebuildTable();
}

void FluxWindow::onRun() {
    int row = m_table->currentRow();
    if (row < 0) { log("Pick a row first.", "#facc15"); return; }
    QTableWidgetItem *it = m_table->item(row, 0);
    if (it->data(RoleSrc).toInt() != 0) {
        log("Install first.", "#facc15");
        return;
    }
    QString id  = it->data(RoleId).toString();
    QString ver = it->data(RoleVer).toString();
    QString proFile;
    for (const auto &r : m_installed) {
        if (r.id == id && r.version == ver) { proFile = r.projectFile; break; }
    }
    if (proFile.isEmpty() || !QFile::exists(proFile)) {
        log("No project file found for " + id + " v" + ver, "#ef4444");
        return;
    }
    log(QString("Run %1 v%2  (%3)").arg(id, ver, proFile), "#5b8cff");
    auto *picker = new ActivityPicker(proFile, this);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    picker->show();
}

void FluxWindow::log(const QString &line, const QString &color) {
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
