#include "MainWindow.h"
#include "SettingsDialog.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QSettings>
#include <QMessageBox>
#include <QDateTime>
#include <QTextCursor>

namespace nx {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
        m_client(new CoreClient(this)) {
    setWindowTitle("Nexor Command");
    resize(1100, 700);

    setupUi();
    buildMenus();

    connect(m_client, &CoreClient::packagesReceived,
            this, &MainWindow::onPackagesReceived);
    connect(m_client, &CoreClient::operationFinished,
            this, &MainWindow::onOperationFinished);
    connect(m_client, &CoreClient::healthReceived,
            this, &MainWindow::onHealthReceived);

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
        QLabel#title {
            background:#0d0e12; color:#fb923c;
            padding:10px 14px; border-bottom:1px solid #1e2030;
            font-family:"Segoe UI"; font-size:14px; font-weight:600;
            letter-spacing:1px;
        }
        QLabel#health  { color:#8a95a3; font-size:12px; padding-right:14px; }
        QPushButton {
            background:#262932; color:#dce1e7;
            border:1px solid #353945; border-radius:4px;
            padding:5px 14px; font-size:12px;
        }
        QPushButton:hover            { background:#2d3140; border-color:#5b8cff; }
        QPushButton#deployBtn        { background:#1e3a5f; }
        QPushButton#deployBtn:hover  { background:#26477a; }
        QPushButton#rollbackBtn      { background:#5f1e1e; }
        QPushButton#rollbackBtn:hover{ background:#7a2626; }
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
            font-family:Consolas, "Courier New", monospace; font-size:12px;
        }
        QComboBox {
            background:#262932; color:#dce1e7;
            border:1px solid #353945; border-radius:3px;
            padding:2px 8px; font-size:12px;
        }
    )");

    auto *root = new QWidget;
    auto *col  = new QVBoxLayout(root);
    col->setContentsMargins(0,0,0,0); col->setSpacing(0);

    // Title row with health indicator
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

    // Filter row
    auto *filterRow = new QWidget;
    filterRow->setStyleSheet("background:#1b1d23; border-bottom:1px solid #1e2030;");
    auto *fL = new QHBoxLayout(filterRow);
    fL->setContentsMargins(14, 8, 14, 8); fL->setSpacing(8);
    fL->addWidget(new QLabel("Filter:"));
    m_filterCombo = new QComboBox;
    m_filterCombo->addItems({"all", "pending", "live", "rolled_back"});
    fL->addWidget(m_filterCombo);
    fL->addStretch();
    m_refreshBtn = new QPushButton("Refresh");
    fL->addWidget(m_refreshBtn);
    col->addWidget(filterRow);

    // Table
    m_table = new QTableWidget(0, 7);
    m_table->setHorizontalHeaderLabels(QStringList()
        << "ID" << "Version" << "Status" << "Title"
        << "Built (UTC)" << "Size (B)" << "Hash");
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(26);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setColumnWidth(0, 160);
    m_table->setColumnWidth(1, 90);
    m_table->setColumnWidth(2, 100);
    m_table->setColumnWidth(3, 220);
    m_table->setColumnWidth(4, 170);
    m_table->setColumnWidth(5, 80);
    col->addWidget(m_table, 1);

    // Action row
    auto *actionRow = new QWidget;
    actionRow->setStyleSheet("background:#1b1d23; border-top:1px solid #1e2030;");
    auto *aL = new QHBoxLayout(actionRow);
    aL->setContentsMargins(14, 8, 14, 8); aL->setSpacing(8);
    m_deployBtn   = new QPushButton("▲ Deploy");
    m_deployBtn  ->setObjectName("deployBtn");
    m_rollbackBtn = new QPushButton("▼ Rollback");
    m_rollbackBtn->setObjectName("rollbackBtn");
    m_settingsBtn = new QPushButton("Settings...");
    aL->addWidget(m_deployBtn);
    aL->addWidget(m_rollbackBtn);
    aL->addStretch();
    aL->addWidget(m_settingsBtn);
    col->addWidget(actionRow);

    // Log
    m_log = new QPlainTextEdit;
    m_log->setObjectName("log");
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);
    m_log->setFixedHeight(180);
    col->addWidget(m_log);

    setCentralWidget(root);
    statusBar()->showMessage("Ready");

    connect(m_refreshBtn,  &QPushButton::clicked, this, &MainWindow::onRefresh);
    connect(m_deployBtn,   &QPushButton::clicked, this, &MainWindow::onDeploy);
    connect(m_rollbackBtn, &QPushButton::clicked, this, &MainWindow::onRollback);
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::onSettings);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ onRefresh(); });
}

void MainWindow::buildMenus() {
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Settings...", this, &MainWindow::onSettings,
                        QKeySequence("Ctrl+,"));
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence("Ctrl+Q"));

    auto *opsMenu = menuBar()->addMenu("&Operations");
    opsMenu->addAction("&Refresh",  this, &MainWindow::onRefresh,  QKeySequence("F5"));
    opsMenu->addAction("&Deploy",   this, &MainWindow::onDeploy);
    opsMenu->addAction("R&ollback", this, &MainWindow::onRollback);

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
    QString filter = m_filterCombo->currentText();
    if (filter == "all") filter.clear();
    log("→ list packages" + (filter.isEmpty() ? QString()
                                              : QString(" (status=%1)").arg(filter)),
        "#5b8cff");
    m_client->listPackages(filter);
    m_client->checkHealth();
}

PackageRow MainWindow::currentRow() const {
    int idx = m_table->currentRow();
    if (idx < 0 || idx >= m_rows.size()) return {};
    return m_rows.at(idx);
}

void MainWindow::onDeploy() {
    PackageRow r = currentRow();
    if (r.id.isEmpty()) { log("Select a row first.", "#facc15"); return; }
    if (QMessageBox::question(this, "Deploy",
            QString("Deploy %1 v%2?\n\nAny currently-live version of '%1' "
                    "will be demoted to rolled_back.").arg(r.id, r.version))
        != QMessageBox::Yes) return;
    log(QString("→ deploy %1 v%2").arg(r.id, r.version), "#5b8cff");
    m_client->deploy(r.id, r.version);
}

void MainWindow::onRollback() {
    PackageRow r = currentRow();
    if (r.id.isEmpty()) { log("Select a row first.", "#facc15"); return; }
    if (r.status != "live") {
        log(QString("Only live versions can be rolled back; %1 v%2 is %3.")
                .arg(r.id, r.version, r.status), "#facc15");
        return;
    }
    if (QMessageBox::question(this, "Rollback",
            QString("Roll back %1 v%2?  Status will become 'rolled_back'.")
                .arg(r.id, r.version))
        != QMessageBox::Yes) return;
    log(QString("→ rollback %1 v%2").arg(r.id, r.version), "#5b8cff");
    m_client->rollback(r.id, r.version);
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

void MainWindow::onPackagesReceived(const QVector<PackageRow> &rows) {
    m_rows = rows;
    m_table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const auto &r = rows.at(i);
        auto put = [&](int col, const QString &text, const QColor &fg = QColor()) {
            auto *it = new QTableWidgetItem(text);
            if (fg.isValid()) it->setForeground(fg);
            m_table->setItem(i, col, it);
        };
        put(0, r.id);
        put(1, r.version);
        QColor statusColor =
            (r.status == "live")        ? QColor("#22c55e") :
            (r.status == "pending")     ? QColor("#facc15") :
            (r.status == "rolled_back") ? QColor("#ef4444") : QColor("#8a95a3");
        put(2, r.status, statusColor);
        put(3, r.title);
        put(4, r.builtAt);
        put(5, QString::number(r.byteSize));
        put(6, r.hash.left(16) + (r.hash.size() > 16 ? "…" : QString()));
    }
    log(QString("← %1 row(s)").arg(rows.size()), "#22c55e");
    statusBar()->showMessage(QString("Loaded %1 row(s) at %2").arg(rows.size())
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void MainWindow::onOperationFinished(const QString &op, bool ok,
                                     const QString &message) {
    log((ok ? "← " : "✗ ") + message, ok ? "#22c55e" : "#ef4444");
    if (ok && (op == "deploy" || op == "rollback")) {
        // Refresh the table so the user sees the new status without
        // having to hit Refresh manually.
        onRefresh();
    }
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
        html += QString("<span style='color:%1'>%2</span>").arg(color, line.toHtmlEscaped());
    else
        html += line.toHtmlEscaped();
    QTextCursor c(m_log->document());
    c.movePosition(QTextCursor::End);
    c.insertHtml(html + "<br>");
    m_log->setTextCursor(c);
}

} // namespace nx
