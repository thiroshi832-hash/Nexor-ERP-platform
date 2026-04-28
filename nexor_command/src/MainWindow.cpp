#include "MainWindow.h"
#include "SettingsDialog.h"
#include "DiffDialog.h"
#include "AuditDialog.h"

#include <QTreeWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDateTime>
#include <QTextCursor>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QSplitter>

namespace nx {

namespace {
QColor statusColour(const QString &s) {
    if (s == "live")        return QColor("#22c55e");
    if (s == "pending")     return QColor("#facc15");
    if (s == "rolled_back") return QColor("#ef4444");
    return QColor("#8a95a3");
}
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
        m_client(new CoreClient(this)),
        m_diffDialog(new DiffDialog(this)),
        m_auditDialog(new AuditDialog(this)) {
    setWindowTitle("Nexor Command");
    resize(1280, 760);

    setupUi();
    buildMenus();

    connect(m_client, &CoreClient::packagesReceived,  this, &MainWindow::onPackagesReceived);
    connect(m_client, &CoreClient::historyReceived,   this, &MainWindow::onHistoryReceived);
    connect(m_client, &CoreClient::auditReceived,     this, &MainWindow::onAuditReceived);
    connect(m_client, &CoreClient::diffReceived,      this, &MainWindow::onDiffReceived);
    connect(m_client, &CoreClient::downloadFinished,  this, &MainWindow::onDownloadFinished);
    connect(m_client, &CoreClient::operationFinished, this, &MainWindow::onOperationFinished);
    connect(m_client, &CoreClient::healthReceived,    this, &MainWindow::onHealthReceived);

    loadSettings();
    log("Nexor Command started.", "#5b8cff");
    m_client->checkHealth();
    onRefresh();
}

void MainWindow::setupUi() {
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
            background:#0d0e12; color:#fb923c;
            padding:10px 14px; border-bottom:1px solid #1e2030;
            font-family:"Segoe UI"; font-size:14px; font-weight:600;
            letter-spacing:1px;
        }
        QLabel#section {
            background:#0d0e12; color:#8a95a3;
            padding:6px 12px; border-bottom:1px solid #1e2030;
            font-size:11px; font-weight:600; letter-spacing:2px;
        }
        QLabel#detTitle {
            color:#dce1e7; font-family:"Segoe UI";
            font-size:18px; font-weight:600; padding:14px 14px 4px 14px;
        }
        QLabel#detStatus  { padding-left:14px; font-size:13px; }
        QLabel#detMeta    { padding:4px 14px; color:#8a95a3; font-size:12px; }
        QLabel#detHash    { padding:2px 14px; color:#dce1e7;
                            font-family:Consolas, "Courier New"; font-size:12px; }
        QLabel#detSig     { padding:2px 14px; color:#a3e635;
                            font-family:Consolas, "Courier New"; font-size:12px; }
        QLabel#health     { color:#8a95a3; font-size:12px; padding-right:14px; }
        QPushButton {
            background:#262932; color:#dce1e7;
            border:1px solid #353945; border-radius:4px;
            padding:5px 14px; font-size:12px;
        }
        QPushButton:hover            { background:#2d3140; border-color:#5b8cff; }
        QPushButton:disabled         { color:#4b5260; border-color:#2a2d35; }
        QPushButton#deployBtn        { background:#1e3a5f; }
        QPushButton#deployBtn:hover  { background:#26477a; }
        QPushButton#rollbackBtn      { background:#5f1e1e; }
        QPushButton#rollbackBtn:hover{ background:#7a2626; }
        QPushButton#deleteBtn        { background:#3a1e1e; color:#fb923c; }
        QPushButton#deleteBtn:hover  { background:#5f2626; }
        QTreeWidget, QTableWidget {
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
        QTreeWidget::item:selected, QTableWidget::item:selected {
            background:#1e3a5f; color:#ffffff;
        }
        QTabBar::tab {
            background:#1b1d23; color:#8a95a3;
            padding:6px 14px; border:none; border-right:1px solid #1e2030;
        }
        QTabBar::tab:selected { color:#fb923c; background:#13151b; }
        QTabWidget::pane     { border:none; }
        QPlainTextEdit#log {
            background:#0d0e12; color:#dce1e7;
            border:none; border-top:1px solid #1e2030;
            font-family:Consolas, "Courier New"; font-size:12px;
        }
    )");

    auto *root = new QWidget;
    auto *col = new QVBoxLayout(root);
    col->setContentsMargins(0,0,0,0); col->setSpacing(0);

    // Title strip + health dot
    auto *titleRow = new QWidget;
    auto *trL = new QHBoxLayout(titleRow);
    trL->setContentsMargins(0,0,0,0); trL->setSpacing(0);
    m_titleLabel = new QLabel("NEXOR COMMAND");
    m_titleLabel->setObjectName("title");
    trL->addWidget(m_titleLabel, 1);
    m_healthDot = new QLabel("● disconnected");
    m_healthDot->setObjectName("health");
    trL->addWidget(m_healthDot);
    col->addWidget(titleRow);

    // Master/Detail body
    auto *split = new QSplitter(Qt::Horizontal);
    split->setHandleWidth(1);
    split->setChildrenCollapsible(false);
    split->setStyleSheet("QSplitter::handle{background:#1e2030;}");

    // ── Master: tree of (project → version)
    auto *masterW = new QWidget;
    auto *masterL = new QVBoxLayout(masterW);
    masterL->setContentsMargins(0,0,0,0); masterL->setSpacing(0);
    auto *masterHdr = new QLabel("PACKAGES"); masterHdr->setObjectName("section");
    masterL->addWidget(masterHdr);
    m_tree = new QTreeWidget;
    m_tree->setColumnCount(3);
    m_tree->setHeaderLabels({"Project / Version", "Status", "Built (UTC)"});
    m_tree->setColumnWidth(0, 220);
    m_tree->setColumnWidth(1, 100);
    m_tree->header()->setStretchLastSection(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setIndentation(14);
    masterL->addWidget(m_tree, 1);
    auto *refreshRow = new QWidget;
    refreshRow->setStyleSheet("background:#1b1d23; border-top:1px solid #1e2030;");
    auto *rL = new QHBoxLayout(refreshRow);
    rL->setContentsMargins(8,6,8,6);
    auto *refreshBtn = new QPushButton("Refresh");
    rL->addWidget(refreshBtn); rL->addStretch();
    masterL->addWidget(refreshRow);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefresh);
    split->addWidget(masterW);

    // ── Detail: header + tabs (manifest / history / audit)
    auto *detailW = new QWidget;
    auto *detL = new QVBoxLayout(detailW);
    detL->setContentsMargins(0,0,0,0); detL->setSpacing(0);

    m_detailStack = new QStackedWidget;

    // Page 0: empty placeholder
    auto *emptyW = new QWidget;
    auto *emptyL = new QVBoxLayout(emptyW);
    emptyL->setAlignment(Qt::AlignCenter);
    auto *emptyLbl = new QLabel("Select a package version to inspect.");
    emptyLbl->setStyleSheet("color:#6b7280; font-size:14px;");
    emptyL->addWidget(emptyLbl);
    m_detailStack->addWidget(emptyW);

    // Page 1: full version detail
    auto *detPage = new QWidget;
    auto *detPL = new QVBoxLayout(detPage);
    detPL->setContentsMargins(0,0,0,0); detPL->setSpacing(0);

    m_detailTitle  = new QLabel; m_detailTitle ->setObjectName("detTitle");
    m_detailStatus = new QLabel; m_detailStatus->setObjectName("detStatus");
    m_detailMeta   = new QLabel; m_detailMeta  ->setObjectName("detMeta");
    m_detailHash   = new QLabel; m_detailHash  ->setObjectName("detHash");
    m_detailSig    = new QLabel; m_detailSig   ->setObjectName("detSig");
    detPL->addWidget(m_detailTitle);
    detPL->addWidget(m_detailStatus);
    detPL->addWidget(m_detailMeta);
    detPL->addWidget(m_detailHash);
    detPL->addWidget(m_detailSig);

    auto *btnRow = new QWidget;
    btnRow->setStyleSheet("background:#1b1d23;");
    auto *btnL = new QHBoxLayout(btnRow);
    btnL->setContentsMargins(14, 8, 14, 8); btnL->setSpacing(8);
    m_deployBtn   = new QPushButton("▲ Deploy");    m_deployBtn  ->setObjectName("deployBtn");
    m_rollbackBtn = new QPushButton("▼ Rollback");  m_rollbackBtn->setObjectName("rollbackBtn");
    m_compareBtn  = new QPushButton("⇄ Compare with…");
    m_downloadBtn = new QPushButton("⤓ Download…");
    m_deleteBtn   = new QPushButton("🗑 Delete");    m_deleteBtn ->setObjectName("deleteBtn");
    btnL->addWidget(m_deployBtn);
    btnL->addWidget(m_rollbackBtn);
    btnL->addWidget(m_compareBtn);
    btnL->addWidget(m_downloadBtn);
    btnL->addWidget(m_deleteBtn);
    btnL->addStretch();
    detPL->addWidget(btnRow);

    m_tabs = new QTabWidget;
    // History tab
    m_historyTable = new QTableWidget(0, 5);
    m_historyTable->setHorizontalHeaderLabels(
        QStringList() << "Version" << "Status" << "Built (UTC)" << "Received (UTC)" << "Bytes");
    m_historyTable->verticalHeader()->setVisible(false);
    m_historyTable->verticalHeader()->setDefaultSectionSize(24);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setColumnWidth(0, 100);
    m_historyTable->setColumnWidth(1, 100);
    m_historyTable->setColumnWidth(2, 170);
    m_historyTable->setColumnWidth(3, 170);
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_tabs->addTab(m_historyTable, "History");

    // Audit tab
    m_auditTable = new QTableWidget(0, 5);
    m_auditTable->setHorizontalHeaderLabels(
        QStringList() << "When (UTC)" << "Event" << "Version" << "Actor" << "Detail");
    m_auditTable->verticalHeader()->setVisible(false);
    m_auditTable->verticalHeader()->setDefaultSectionSize(24);
    m_auditTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_auditTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_auditTable->setColumnWidth(0, 170);
    m_auditTable->setColumnWidth(1, 110);
    m_auditTable->setColumnWidth(2, 100);
    m_auditTable->setColumnWidth(3, 110);
    m_auditTable->horizontalHeader()->setStretchLastSection(true);
    m_tabs->addTab(m_auditTable, "Audit");

    detPL->addWidget(m_tabs, 1);
    m_detailStack->addWidget(detPage);

    detL->addWidget(m_detailStack, 1);
    split->addWidget(detailW);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setSizes({ 320, 960 });
    col->addWidget(split, 1);

    // Log strip
    m_log = new QPlainTextEdit;
    m_log->setObjectName("log");
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);
    m_log->setFixedHeight(140);
    col->addWidget(m_log);

    setCentralWidget(root);
    statusBar()->showMessage("Ready");

    connect(m_tree, &QTreeWidget::itemSelectionChanged,
            this, &MainWindow::onTreeSelectionChanged);
    connect(m_deployBtn,   &QPushButton::clicked, this, &MainWindow::onDeploy);
    connect(m_rollbackBtn, &QPushButton::clicked, this, &MainWindow::onRollback);
    connect(m_compareBtn,  &QPushButton::clicked, this, &MainWindow::onCompareWith);
    connect(m_downloadBtn, &QPushButton::clicked, this, &MainWindow::onDownload);
    connect(m_deleteBtn,   &QPushButton::clicked, this, &MainWindow::onDeletePending);

    clearVersion();
}

void MainWindow::buildMenus() {
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Settings...", this, &MainWindow::onSettings,
                        QKeySequence("Ctrl+,"));
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence("Ctrl+Q"));

    auto *opsMenu = menuBar()->addMenu("&Operations");
    opsMenu->addAction("&Register Package...", this, &MainWindow::onRegisterPackage,
                       QKeySequence("Ctrl+R"));
    opsMenu->addSeparator();
    opsMenu->addAction("&Refresh",  this, &MainWindow::onRefresh,  QKeySequence("F5"));
    opsMenu->addAction("&Deploy",   this, &MainWindow::onDeploy);
    opsMenu->addAction("R&ollback", this, &MainWindow::onRollback);
    opsMenu->addSeparator();
    opsMenu->addAction("&Compare with...",      this, &MainWindow::onCompareWith);
    opsMenu->addAction("D&ownload...",          this, &MainWindow::onDownload);
    opsMenu->addAction("Dele&te (pending)",     this, &MainWindow::onDeletePending);

    auto *auditMenu = menuBar()->addMenu("&Audit");
    auditMenu->addAction("Show full audit log...",        this, &MainWindow::onAuditAll);
    auditMenu->addAction("Show audit for selection...",   this, &MainWindow::onAuditForSelected);

    auto *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About", this, [this]{
        QMessageBox::about(this, "About Nexor Command",
            "<h2>Nexor Command</h2>"
            "<p>Version 0.1.0 — package deployment console.</p>"
            "<p>Talks to a Nexor Core instance at the configured URL.</p>");
    });
}

void MainWindow::loadSettings() {
    QSettings s;
    QString url = s.value("Core/Url",   "http://localhost:7421").toString();
    QString tok = s.value("Core/Token", "").toString();
    m_client->setBaseUrl(url);
    m_client->setAdminToken(tok);
    m_titleLabel->setText("NEXOR COMMAND   —   Core: " + url
                          + (tok.isEmpty() ? "   (open)" : "   (authed)"));
}

void MainWindow::onRefresh() {
    log("→ list packages", "#5b8cff");
    m_client->listPackages();        // unfiltered
    m_client->checkHealth();
}

void MainWindow::onSettings() {
    QSettings s;
    SettingsDialog dlg(this);
    dlg.setCoreUrl   (s.value("Core/Url",   "http://localhost:7421").toString());
    dlg.setAdminToken(s.value("Core/Token", "").toString());
    if (dlg.exec() != QDialog::Accepted) return;
    s.setValue("Core/Url",   dlg.coreUrl());
    s.setValue("Core/Token", dlg.adminToken());
    loadSettings();
    log("Connection settings saved.", "#8a95a3");
    m_client->checkHealth();
    onRefresh();
}

void MainWindow::onRegisterPackage() {
    // Pick a .nexor file from disk and POST it to Core.  This is Command's
    // bridge between Studio's local Build output and the central registry -
    // Studio writes the file, an administrator running Command uploads it.
    QSettings s;
    QString lastDir = s.value("Register/LastDir").toString();
    QString path = QFileDialog::getOpenFileName(this,
        "Register Nexor Package",
        lastDir,
        "Nexor Package (*.nexor);;All files (*.*)");
    if (path.isEmpty()) return;
    s.setValue("Register/LastDir", QFileInfo(path).absolutePath());

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        log("Cannot read " + path, "#ef4444");
        return;
    }
    QByteArray bytes = f.readAll();
    f.close();
    log(QString("→ register %1 (%2 bytes) → %3")
            .arg(QFileInfo(path).fileName())
            .arg(bytes.size())
            .arg(m_client->baseUrl()),
        "#5b8cff");
    m_client->registerPackage(bytes, path);
}

PackageRow MainWindow::currentVersion() const { return m_selectedVersion; }
QString    MainWindow::currentProjectId() const {
    if (!m_selectedVersion.id.isEmpty()) return m_selectedVersion.id;
    QTreeWidgetItem *it = m_tree->currentItem();
    if (!it) return {};
    if (it->parent() == nullptr) return it->text(0);
    return it->parent()->text(0);
}

void MainWindow::onTreeSelectionChanged() {
    QTreeWidgetItem *it = m_tree->currentItem();
    if (!it) { clearVersion(); return; }
    if (it->parent() == nullptr) {
        // Project node — show history of all versions.
        QString id = it->text(0);
        m_selectedVersion = {};
        clearVersion();
        m_detailStack->setCurrentIndex(1);
        m_detailTitle->setText(id);
        m_detailStatus->setText(QString("%1 version(s)")
                                .arg(m_versionsById.value(id).size()));
        m_detailMeta->setText("Select a version below to operate on it.");
        m_detailHash->clear(); m_detailSig->clear();
        m_deployBtn->setEnabled(false);
        m_rollbackBtn->setEnabled(false);
        m_compareBtn->setEnabled(false);
        m_downloadBtn->setEnabled(false);
        m_deleteBtn  ->setEnabled(false);
        m_client->historyOf(id);
        m_client->auditLog(id);
        return;
    }
    // Version node
    QString id  = it->parent()->text(0);
    QString ver = it->text(0);
    for (const auto &r : m_versionsById.value(id))
        if (r.version == ver) { showVersion(r); return; }
}

void MainWindow::showVersion(const PackageRow &r) {
    m_selectedVersion = r;
    m_detailStack->setCurrentIndex(1);
    m_detailTitle ->setText(r.id + " / " + r.version);
    m_detailStatus->setText("Status:  " + r.status);
    QPalette pal = m_detailStatus->palette();
    pal.setColor(QPalette::WindowText, statusColour(r.status));
    m_detailStatus->setPalette(pal);
    m_detailStatus->setStyleSheet(
        QString("padding-left:14px; font-size:13px; color:%1;")
            .arg(statusColour(r.status).name()));
    m_detailMeta->setText(QString("Title: %1     Built: %2     Received: %3     Bytes: %4")
        .arg(r.title.isEmpty() ? "(none)" : r.title)
        .arg(r.builtAt).arg(r.receivedAt)
        .arg(QString::number(r.byteSize)));
    m_detailHash->setText("Hash: " + r.hash);
    m_detailSig ->setText(QString());     // signature info not in list endpoint (yet)

    bool isPending     = (r.status == "pending");
    bool isLive        = (r.status == "live");
    m_deployBtn  ->setEnabled(!isLive);
    m_rollbackBtn->setEnabled(isLive);
    m_compareBtn ->setEnabled(true);
    m_downloadBtn->setEnabled(true);
    m_deleteBtn  ->setEnabled(isPending);

    m_client->historyOf(r.id);
    m_client->auditLog(r.id);
}

void MainWindow::clearVersion() {
    m_selectedVersion = {};
    m_detailStack->setCurrentIndex(0);
    m_deployBtn  ->setEnabled(false);
    m_rollbackBtn->setEnabled(false);
    m_compareBtn ->setEnabled(false);
    m_downloadBtn->setEnabled(false);
    m_deleteBtn  ->setEnabled(false);
}

void MainWindow::onDeploy() {
    PackageRow r = currentVersion();
    if (r.id.isEmpty()) return;
    if (QMessageBox::question(this, "Deploy",
            QString("Deploy %1 v%2?\n\nAny currently-live version of '%1' "
                    "will be demoted to rolled_back.").arg(r.id, r.version))
        != QMessageBox::Yes) return;
    log(QString("→ deploy %1 v%2").arg(r.id, r.version), "#5b8cff");
    m_client->deploy(r.id, r.version);
}

void MainWindow::onRollback() {
    PackageRow r = currentVersion();
    if (r.id.isEmpty() || r.status != "live") return;
    if (QMessageBox::question(this, "Rollback",
            QString("Roll back %1 v%2?  Status will become 'rolled_back'.")
                .arg(r.id, r.version))
        != QMessageBox::Yes) return;
    log(QString("→ rollback %1 v%2").arg(r.id, r.version), "#5b8cff");
    m_client->rollback(r.id, r.version);
}

void MainWindow::onDownload() {
    PackageRow r = currentVersion();
    if (r.id.isEmpty()) return;
    QString suggest = QString("%1-%2.nexor").arg(r.id, r.version);
    QString dest = QFileDialog::getSaveFileName(this, "Download Package",
        suggest, "Nexor Package (*.nexor)");
    if (dest.isEmpty()) return;
    log(QString("→ download %1 v%2 → %3").arg(r.id, r.version, dest), "#5b8cff");
    m_client->downloadTo(r.id, r.version, dest);
}

void MainWindow::onDeletePending() {
    PackageRow r = currentVersion();
    if (r.id.isEmpty() || r.status != "pending") return;
    if (QMessageBox::question(this, "Delete pending",
            QString("Permanently delete pending package %1 v%2?\n\n"
                    "The .nexor file is removed from disk and the row is "
                    "dropped from the registry.  An audit event is written.")
                .arg(r.id, r.version))
        != QMessageBox::Yes) return;
    log(QString("→ delete-pending %1 v%2").arg(r.id, r.version), "#5b8cff");
    m_client->deletePending(r.id, r.version);
}

void MainWindow::onCompareWith() {
    PackageRow r = currentVersion();
    if (r.id.isEmpty()) return;

    const auto versions = m_versionsById.value(r.id);
    QStringList opts;
    for (const auto &v : versions)
        if (v.version != r.version) opts.append(v.version);
    if (opts.isEmpty()) {
        log(QString("No other versions of %1 to compare against.").arg(r.id), "#facc15");
        return;
    }
    bool ok = false;
    QString other = QInputDialog::getItem(this,
        "Compare with", QString("Compare %1 v%2 with:").arg(r.id, r.version),
        opts, 0, false, &ok);
    if (!ok || other.isEmpty()) return;
    log(QString("→ diff %1 %2 vs %3").arg(r.id, other, r.version), "#5b8cff");
    m_client->diff(r.id, other, r.version);
}

void MainWindow::onAuditAll() {
    log("→ audit (all)", "#5b8cff");
    m_routeNextAuditToDialog  = true;
    m_pendingAuditDialogScope.clear();
    m_client->auditLog();
}

void MainWindow::onAuditForSelected() {
    QString id = currentProjectId();
    if (id.isEmpty()) { log("Select a project or version first.", "#facc15"); return; }
    log(QString("→ audit (%1)").arg(id), "#5b8cff");
    m_routeNextAuditToDialog  = true;
    m_pendingAuditDialogScope = id;
    m_client->auditLog(id);
}

void MainWindow::onPackagesReceived(const QVector<PackageRow> &rows) {
    // Group by id for the master tree.
    m_versionsById.clear();
    for (const auto &r : rows) m_versionsById[r.id].append(r);

    QString prevSelectionId  = currentProjectId();
    QString prevSelectionVer = m_selectedVersion.version;

    m_tree->clear();
    QStringList ids = m_versionsById.keys();
    std::sort(ids.begin(), ids.end());
    for (const QString &id : ids) {
        auto *parent = new QTreeWidgetItem(m_tree, QStringList() << id);
        QFont f = parent->font(0); f.setBold(true);
        parent->setFont(0, f);
        // Show count + live version in the status column.
        int liveCount = 0; QString liveVer;
        for (const auto &v : m_versionsById.value(id))
            if (v.status == "live") { ++liveCount; liveVer = v.version; }
        parent->setText(1, liveCount > 0 ? ("live: " + liveVer)
                                         : QString::number(m_versionsById.value(id).size()) + " ver");
        parent->setForeground(1, liveCount > 0 ? QColor("#22c55e") : QColor("#8a95a3"));
        for (const auto &v : m_versionsById.value(id)) {
            auto *it = new QTreeWidgetItem(parent, QStringList()
                << v.version << v.status << v.builtAt);
            it->setForeground(1, statusColour(v.status));
            if (v.status == "live") {
                QFont ff = it->font(0); ff.setBold(true);
                it->setFont(0, ff);
            }
        }
        parent->setExpanded(true);
    }

    // Restore selection if possible.
    bool restored = false;
    for (int i = 0; i < m_tree->topLevelItemCount() && !restored; ++i) {
        QTreeWidgetItem *p = m_tree->topLevelItem(i);
        if (p->text(0) != prevSelectionId) continue;
        if (prevSelectionVer.isEmpty()) {
            m_tree->setCurrentItem(p);
            restored = true;
            break;
        }
        for (int j = 0; j < p->childCount(); ++j) {
            if (p->child(j)->text(0) == prevSelectionVer) {
                m_tree->setCurrentItem(p->child(j));
                restored = true;
                break;
            }
        }
    }
    log(QString("← %1 row(s)").arg(rows.size()), "#22c55e");
    statusBar()->showMessage(QString("Loaded %1 row(s) at %2").arg(rows.size())
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void MainWindow::onHistoryReceived(const QString &id, const QVector<PackageRow> &rows) {
    if (currentProjectId() != id) return;
    m_historyTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const auto &r = rows.at(i);
        auto put = [&](int col, const QString &text, const QColor &fg = QColor()) {
            auto *it = new QTableWidgetItem(text);
            if (fg.isValid()) it->setForeground(fg);
            m_historyTable->setItem(i, col, it);
        };
        put(0, r.version);
        put(1, r.status, statusColour(r.status));
        put(2, r.builtAt);
        put(3, r.receivedAt);
        put(4, QString::number(r.byteSize));
    }
}

void MainWindow::onAuditReceived(const QVector<AuditRow> &events) {
    if (m_routeNextAuditToDialog) {
        m_routeNextAuditToDialog = false;
        m_auditDialog->setEvents(m_pendingAuditDialogScope, events);
        m_auditDialog->show();
        m_auditDialog->raise();
        return;
    }
    m_auditTable->setRowCount(events.size());
    for (int i = 0; i < events.size(); ++i) {
        const auto &e = events.at(i);
        auto put = [&](int col, const QString &text, const QColor &fg = QColor()) {
            auto *it = new QTableWidgetItem(text);
            if (fg.isValid()) it->setForeground(fg);
            m_auditTable->setItem(i, col, it);
        };
        QColor c =
            (e.eventType == "publish")        ? QColor("#5b8cff") :
            (e.eventType == "deploy")         ? QColor("#22c55e") :
            (e.eventType == "rollback")       ? QColor("#ef4444") :
            (e.eventType == "delete-pending") ? QColor("#fb923c") :
                                                QColor("#dce1e7");
        put(0, e.occurredAt);
        put(1, e.eventType, c);
        put(2, e.version);
        put(3, e.actor);
        put(4, e.detail);
    }
}

void MainWindow::onDiffReceived(const DiffResult &diff) {
    m_diffDialog->setDiff(diff);
    m_diffDialog->show();
    m_diffDialog->raise();
}

void MainWindow::onDownloadFinished(const QString &id, const QString &version,
                                    bool ok, const QString &localPath,
                                    const QString &message) {
    log((ok ? "← " : "✗ ") + message, ok ? "#22c55e" : "#ef4444");
    Q_UNUSED(id); Q_UNUSED(version); Q_UNUSED(localPath);
}

void MainWindow::onOperationFinished(const QString &op, bool ok,
                                     const QString &message) {
    log((ok ? "← " : "✗ ") + message, ok ? "#22c55e" : "#ef4444");
    if (ok && (op == "deploy" || op == "rollback" ||
               op == "delete-pending" || op == "register"))
        onRefresh();
}

void MainWindow::onHealthReceived(bool ok, const QString &info) {
    if (ok) {
        m_healthDot->setText("● live");
        m_healthDot->setStyleSheet("color:#22c55e; font-size:12px; padding-right:14px;");
    } else {
        m_healthDot->setText("● unreachable");
        m_healthDot->setStyleSheet("color:#ef4444; font-size:12px; padding-right:14px;");
        log("health: " + info, "#ef4444");
    }
}

void MainWindow::log(const QString &line, const QString &color) {
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
