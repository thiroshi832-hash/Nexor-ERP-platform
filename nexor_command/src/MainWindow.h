// =============================================================================
// Nexor Command — package-deployment console (Phase 11 master/detail).
//
//   ┌────────────────────────────────────────────────────────────────────┐
//   │ NEXOR COMMAND — Core: http://localhost:7421     [● live]           │
//   ├──────────────┬──────────────────────────────────────────┬──────────┤
//   │ PACKAGES     │ Sales / 0.4.1                            │ Manifest │
//   │  ▼ Sales     │  Status: pending                         │ History  │
//   │     0.4.1    │  Title:  Sales                           │ Audit    │
//   │     0.4.0 *  │  Built:  2026-04-29T03:50Z               │          │
//   │  ▼ Inventory │  Hash:   a3f9…                           │          │
//   │     1.0.0    │  Sig:    hmac-sha256 7c2b…               │          │
//   │              │  Bytes:  1,247,392                       │          │
//   │              │                                          │          │
//   │              │ [ Deploy ] [ Rollback ] [ Compare with…] │          │
//   │              │ [ Download…] [ Delete pending ]          │          │
//   ├──────────────┴──────────────────────────────────────────┴──────────┤
//   │ 12:34:01 → list packages                                           │
//   │ 12:34:01 ← 5 row(s)                                                │
//   └────────────────────────────────────────────────────────────────────┘
//
// "live" rows wear a green dot; "pending" rows are yellow; "rolled_back"
// is red.  The comparison sub-menu picks the second version from the same
// project's history.
// =============================================================================
#ifndef NEXOR_COMMAND_MAINWINDOW_H
#define NEXOR_COMMAND_MAINWINDOW_H

#include <QMainWindow>
#include <QHash>
#include "CoreClient.h"

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPushButton;
class QPlainTextEdit;
class QStackedWidget;
class QTabWidget;
class QTableWidget;

namespace nx {

class DiffDialog;
class AuditDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onSettings();
    void onRegisterPackage();
    void onAuditAll();
    void onAuditForSelected();
    void onTreeSelectionChanged();
    void onDeploy();
    void onRollback();
    void onDownload();
    void onDeletePending();
    void onCompareWith();

    void onPackagesReceived (const QVector<PackageRow> &rows);
    void onHistoryReceived  (const QString &id, const QVector<PackageRow> &rows);
    void onAuditReceived    (const QVector<AuditRow> &events);
    void onDiffReceived     (const DiffResult &diff);
    void onDownloadFinished (const QString &id, const QString &version,
                             bool ok, const QString &localPath, const QString &message);
    void onOperationFinished(const QString &op, bool ok, const QString &message);
    void onHealthReceived   (bool ok, const QString &info);

private:
    void setupUi();
    void buildMenus();
    void loadSettings();
    void log(const QString &line, const QString &color = QString());
    void showVersion (const PackageRow &r);
    void clearVersion();
    PackageRow currentVersion() const;     // NULL row if a project node is selected
    QString    currentProjectId() const;   // works for both project and version selection

    CoreClient    *m_client;
    DiffDialog    *m_diffDialog;
    AuditDialog   *m_auditDialog;

    QLabel        *m_titleLabel;
    QLabel        *m_healthDot;

    QTreeWidget   *m_tree;          // master
    QStackedWidget *m_detailStack;
    // detail pane widgets
    QLabel        *m_detailTitle;
    QLabel        *m_detailStatus;
    QLabel        *m_detailMeta;       // built / received / size
    QLabel        *m_detailHash;
    QLabel        *m_detailSig;
    QPushButton   *m_deployBtn;
    QPushButton   *m_rollbackBtn;
    QPushButton   *m_compareBtn;
    QPushButton   *m_downloadBtn;
    QPushButton   *m_deleteBtn;

    QTabWidget    *m_tabs;
    QTableWidget  *m_historyTable;     // populated when a project is selected
    QTableWidget  *m_auditTable;       // package-scoped audit
    QPlainTextEdit *m_log;

    // Cached state — Core tells us about the world, we render it.
    QHash<QString, QVector<PackageRow>> m_versionsById;     // id -> versions list
    PackageRow m_selectedVersion;
    // When set, the next onAuditReceived event populates the AuditDialog
    // instead of the in-pane Audit tab — avoids a single shared signal
    // overwriting the tab when the user opens the global audit window.
    bool       m_routeNextAuditToDialog { false };
    QString    m_pendingAuditDialogScope;
};

} // namespace nx

#endif // NEXOR_COMMAND_MAINWINDOW_H
