#include "MainWindow.h"
#include "SettingsDialog.h"
#include "Http.h"
#include "PackageRegistry.h"
#include "CoreEntityStore.h"
#include "PackageApi.h"
#include "RpcApi.h"
#include "EntityApi.h"
#include "ProcessApi.h"
#include "../../nexor_studio/src/build/PackageReader.h"

#include <QLabel>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QTextCursor>
#include <QMessageBox>
#include <QAction>

namespace nx {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Nexor Core");
    resize(960, 640);

    QSettings s;
    m_port       = static_cast<quint16>(s.value("Server/Port", 7421).toUInt());
    m_dataRoot   = s.value("Server/DataRoot",
        QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
            .absoluteFilePath("Nexor/Core")).toString();
    m_signingKey = s.value("Server/SigningKey").toString();
    m_adminToken = s.value("Server/AdminToken").toString();

    setupUi();
    buildMenus();
    refreshHeader();

    log("Nexor Core started.", "#5b8cff");
}

MainWindow::~MainWindow() {
    stopServer();
}

void MainWindow::applyCliOverrides(const QString &port,
                                   const QString &dataRoot,
                                   const QString &signingKey,
                                   const QString &adminToken) {
    if (!port      .isEmpty()) m_port       = static_cast<quint16>(port.toUInt());
    if (!dataRoot  .isEmpty()) m_dataRoot   = dataRoot;
    if (!signingKey.isEmpty()) m_signingKey = signingKey;
    if (!adminToken.isEmpty()) m_adminToken = adminToken;
    refreshHeader();
}

void MainWindow::setupUi() {
    setStyleSheet(R"(
        QMainWindow { background:#13151b; }
        QMenuBar    { background:#0d0e12; color:#dce1e7;
                      border-bottom:1px solid #1e2030; }
        QMenuBar::item   { padding:5px 12px; }
        QMenuBar::item:selected { background:#3a1e5f; }
        QMenu       { background:#1b1d23; color:#dce1e7;
                      border:1px solid #2a3655; }
        QMenu::item:selected { background:#3a1e5f; }
        QStatusBar  { background:#0d0e12; color:#8a95a3;
                      border-top:1px solid #1e2030; padding:0 8px; }
        QLabel      { color:#dce1e7; }
        QLabel#title {
            background:#0d0e12; color:#c084fc;
            padding:10px 14px; border-bottom:1px solid #1e2030;
            font-family:"Segoe UI"; font-size:14px; font-weight:600;
            letter-spacing:1px;
        }
        QLabel#status { color:#8a95a3; font-size:12px; padding-right:14px; }
        QLabel#meta {
            background:#0d0e12; color:#8a95a3;
            padding:4px 14px;
            font-family:Consolas, "Courier New"; font-size:12px;
        }
        QPlainTextEdit#log {
            background:#0d0e12; color:#dce1e7;
            border:none; border-top:1px solid #1e2030;
            font-family:Consolas, "Courier New"; font-size:12px;
        }
    )");

    auto *root = new QWidget;
    auto *col = new QVBoxLayout(root);
    col->setContentsMargins(0, 0, 0, 0); col->setSpacing(0);

    // Title row + running dot
    auto *titleRow = new QWidget;
    auto *trL = new QHBoxLayout(titleRow);
    trL->setContentsMargins(0, 0, 0, 0); trL->setSpacing(0);
    m_titleLabel = new QLabel("NEXOR CORE");
    m_titleLabel->setObjectName("title");
    trL->addWidget(m_titleLabel, 1);
    m_statusDot = new QLabel("● stopped");
    m_statusDot->setObjectName("status");
    trL->addWidget(m_statusDot);
    col->addWidget(titleRow);

    // Three meta rows showing the current environment.
    m_dataLabel    = new QLabel; m_dataLabel   ->setObjectName("meta");
    m_signingLabel = new QLabel; m_signingLabel->setObjectName("meta");
    m_authLabel    = new QLabel; m_authLabel   ->setObjectName("meta");
    col->addWidget(m_dataLabel);
    col->addWidget(m_signingLabel);
    col->addWidget(m_authLabel);

    // Live log fills the rest.
    m_log = new QPlainTextEdit;
    m_log->setObjectName("log");
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(10000);
    col->addWidget(m_log, 1);

    setCentralWidget(root);
    statusBar()->showMessage("Stopped");
}

void MainWindow::buildMenus() {
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Exit", this, &QWidget::close, QKeySequence("Ctrl+Q"));

    auto *serverMenu = menuBar()->addMenu("&Server");
    m_startStopAction = serverMenu->addAction("&Start", this, &MainWindow::onStartStop);
    m_startStopAction->setShortcut(QKeySequence("F5"));
    serverMenu->addAction("&Restart", this, &MainWindow::onRestart, QKeySequence("Ctrl+R"));
    serverMenu->addSeparator();
    serverMenu->addAction("&Settings...",  this, &MainWindow::onSettings, QKeySequence("Ctrl+,"));
    serverMenu->addAction("Clear &Log",    this, &MainWindow::onClearLog, QKeySequence("Ctrl+L"));

    auto *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About", this, [this]{
        QMessageBox::about(this, "About Nexor Core",
            "<h2>Nexor Core</h2>"
            "<p>Version 0.1.0 — backend server.</p>"
            "<p>Hosts the package registry, entity ORM, RPC, and persistent "
            "workflow engine.</p>");
    });
}

void MainWindow::refreshHeader() {
    m_titleLabel->setText(
        QString("NEXOR CORE   —   http://0.0.0.0:%1").arg(m_port));
    m_dataLabel   ->setText("Data root:  " + m_dataRoot);
    m_signingLabel->setText(
        QString("Signing:    %1").arg(m_signingKey.isEmpty() ? "permissive (no key)"
                                                              : "HMAC-SHA256 key set"));
    m_authLabel   ->setText(
        QString("Admin auth: %1").arg(m_adminToken.isEmpty() ? "open (no bearer required)"
                                                              : "bearer token required"));
}

void MainWindow::onSettings() {
    SettingsDialog dlg(this);
    dlg.setPort       (m_port);
    dlg.setDataRoot   (m_dataRoot);
    dlg.setSigningKey (m_signingKey);
    dlg.setAdminToken (m_adminToken);
    if (dlg.exec() != QDialog::Accepted) return;

    m_port       = dlg.port();
    m_dataRoot   = dlg.dataRoot();
    m_signingKey = dlg.signingKey();
    m_adminToken = dlg.adminToken();
    QSettings s;
    s.setValue("Server/Port",       m_port);
    s.setValue("Server/DataRoot",   m_dataRoot);
    s.setValue("Server/SigningKey", m_signingKey);
    s.setValue("Server/AdminToken", m_adminToken);
    refreshHeader();
    log("Settings saved.  Use Server → Restart to apply.", "#facc15");
}

void MainWindow::onStartStop() {
    if (m_running) stopServer();
    else           startServer();
}

void MainWindow::onRestart() {
    if (m_running) stopServer();
    startServer();
}

void MainWindow::onClearLog() {
    m_log->clear();
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

void MainWindow::startServer() {
    if (m_running) return;
    QString err;

    m_registry = std::make_unique<PackageRegistry>(m_dataRoot);
    if (!m_registry->open(&err)) {
        log("Registry open failed: " + err, "#ef4444");
        m_registry.reset();
        return;
    }
    if (!m_signingKey.isEmpty())
        m_registry->setSigningKey(m_signingKey.toUtf8());

    m_entities = std::make_unique<CoreEntityStore>(m_dataRoot);
    if (!m_entities->open(&err)) {
        log("Entity store open failed: " + err, "#ef4444");
        m_registry.reset();
        m_entities.reset();
        return;
    }

    // Auto-register sheets from already-stored packages + every future
    // upload (package listener fires inside publish()).
    auto registerFromBytes = [this](const QByteArray &bytes){
        auto rd = PackageReader::fromBytes(bytes, false);
        if (rd.status == PackageReader::Status::Ok)
            m_entities->registerSheetsFromPackage(rd.package);
    };
    m_registry->setPackageListener(registerFromBytes);
    for (const auto &row : m_registry->list()) {
        QFile f(row.filePath);
        if (!f.open(QIODevice::ReadOnly)) continue;
        registerFromBytes(f.readAll());
    }

    m_router      = std::make_unique<Router>();
    m_packageApi  = std::make_unique<PackageApi>(m_router.get(), m_registry.get());
    m_rpcApi      = std::make_unique<RpcApi>    (m_router.get(), m_registry.get());
    m_entityApi   = std::make_unique<EntityApi> (m_router.get(), m_entities.get());
    m_processApi  = std::make_unique<ProcessApi>(m_router.get(), m_registry.get());

    m_packageApi->setAdminToken(m_adminToken);
    m_rpcApi    ->setAdminToken(m_adminToken);
    m_entityApi ->setAdminToken(m_adminToken);
    m_processApi->setAdminToken(m_adminToken);
    if (!m_processApi->open(m_dataRoot, &err)) {
        log("Process store open failed: " + err, "#ef4444");
        return;
    }

    m_packageApi->registerRoutes();
    m_rpcApi    ->registerRoutes();
    m_entityApi ->registerRoutes();
    m_processApi->registerRoutes();

    m_server = std::make_unique<HttpServer>(m_router.get());
    if (!m_server->start(m_port)) {
        log(QString("Cannot bind port %1 - already in use?").arg(m_port), "#ef4444");
        m_server.reset();
        return;
    }

    m_running = true;
    m_startStopAction->setText("&Stop");
    m_statusDot->setText("● running");
    m_statusDot->setStyleSheet("color:#22c55e; font-size:12px; padding-right:14px;");
    statusBar()->showMessage(QString("Listening on http://0.0.0.0:%1").arg(m_port));
    log(QString("Server listening on port %1").arg(m_port), "#22c55e");
    log(QString("  data root:   %1").arg(m_dataRoot), "#8a95a3");
    log(QString("  signing-key: %1").arg(m_signingKey.isEmpty() ? "<permissive>" : "<set>"),
        "#8a95a3");
    log(QString("  admin-token: %1").arg(m_adminToken.isEmpty() ? "<open>" : "<set>"),
        "#8a95a3");
}

void MainWindow::stopServer() {
    if (!m_running) return;
    m_server.reset();
    m_processApi.reset();
    m_entityApi.reset();
    m_rpcApi.reset();
    m_packageApi.reset();
    m_router.reset();
    m_entities.reset();
    m_registry.reset();
    m_running = false;
    if (m_startStopAction) m_startStopAction->setText("&Start");
    if (m_statusDot) {
        m_statusDot->setText("● stopped");
        m_statusDot->setStyleSheet("color:#8a95a3; font-size:12px; padding-right:14px;");
    }
    statusBar()->showMessage("Stopped");
    log("Server stopped.", "#8a95a3");
}

} // namespace nx
