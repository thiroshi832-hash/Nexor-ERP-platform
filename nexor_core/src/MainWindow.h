// =============================================================================
// MainWindow — Core's GUI shell.
//
//   ┌──────────────────────────────────────────────────────────────────┐
//   │ NEXOR CORE  —  http://0.0.0.0:7421               [● running]     │
//   ├──────────────────────────────────────────────────────────────────┤
//   │ Data root:  C:\Users\…\AppData\Roaming\Nexor\Core                │
//   │ Signing:    permissive                                            │
//   │ Admin auth: open                                                  │
//   ├──────────────────────────────────────────────────────────────────┤
//   │ 12:34:01 GET  /api/v1/health → 200                                │
//   │ 12:34:05 POST /api/v1/packages → 201                              │
//   │ 12:34:06 POST /api/v1/admin/packages/Sales/0.4.1/deploy → 200     │
//   │ …                                                                 │
//   └──────────────────────────────────────────────────────────────────┘
//
//   Server menu: Start / Stop / Restart / Settings...
//
// The HTTP server (Http + Router + PackageApi + RpcApi + EntityApi +
// ProcessApi) runs in this same process - the GUI is just a settings
// editor + log viewer that sits on top.
// =============================================================================
#ifndef NEXOR_CORE_MAINWINDOW_H
#define NEXOR_CORE_MAINWINDOW_H

#include <QMainWindow>
#include <memory>

class QLabel;
class QPlainTextEdit;
class QAction;

namespace nx {

class HttpServer;
class Router;
class PackageRegistry;
class CoreEntityStore;
class PackageApi;
class RpcApi;
class EntityApi;
class ProcessApi;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Apply CLI overrides on top of QSettings (called once at startup
    // before the server is started).
    void applyCliOverrides(const QString &port,
                           const QString &dataRoot,
                           const QString &signingKey,
                           const QString &adminToken);

    void startServer();          // tear down + reopen with current settings
    void stopServer();
    bool isRunning() const       { return m_running; }

private slots:
    void onSettings();
    void onStartStop();
    void onRestart();
    void onClearLog();

private:
    void setupUi();
    void buildMenus();
    void log(const QString &line, const QString &color = QString());
    void refreshHeader();

    // The set of settings that drive the server.  Persisted to QSettings
    // under "Server/Port", "Server/DataRoot", etc.
    quint16 m_port      { 7421 };
    QString m_dataRoot;
    QString m_signingKey;
    QString m_adminToken;

    // UI bits.
    QLabel         *m_titleLabel;
    QLabel         *m_statusDot;
    QLabel         *m_dataLabel;
    QLabel         *m_signingLabel;
    QLabel         *m_authLabel;
    QPlainTextEdit *m_log;
    QAction        *m_startStopAction;

    // The server instances live here so they can be torn down + reopened.
    bool                                m_running { false };
    std::unique_ptr<Router>             m_router;
    std::unique_ptr<PackageRegistry>    m_registry;
    std::unique_ptr<CoreEntityStore>    m_entities;
    std::unique_ptr<PackageApi>         m_packageApi;
    std::unique_ptr<RpcApi>             m_rpcApi;
    std::unique_ptr<EntityApi>          m_entityApi;
    std::unique_ptr<ProcessApi>         m_processApi;
    std::unique_ptr<HttpServer>         m_server;
};

} // namespace nx

#endif // NEXOR_CORE_MAINWINDOW_H
