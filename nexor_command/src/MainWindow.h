// =============================================================================
// Nexor Command — package-deployment console.
//
//   ┌──────────────────────────────────────────────────────────────────┐
//   │  ▼ NEXOR COMMAND — Core: http://localhost:7421       (● live)    │
//   │  Filter: [ All  Pending  Live  Rolled-back ]  [ Refresh ]        │
//   ├──────────────────────────────────────────────────────────────────┤
//   │  ID         Version   Status     Built          Hash             │
//   │  Sales      0.4.1     pending    2026-04-29     a3f9…            │
//   │  Sales      0.4.0     live       2026-04-21     1cb2…            │
//   │  Inventory  1.0.0     rolled…    2026-04-12     7d22…            │
//   ├──────────────────────────────────────────────────────────────────┤
//   │  [ Deploy ]   [ Rollback ]   [ Settings... ]                     │
//   ├──────────────────────────────────────────────────────────────────┤
//   │  >> deploy Sales 0.4.1 OK                                        │
//   │  >> list returned 3 rows                                         │
//   └──────────────────────────────────────────────────────────────────┘
//
// Backed by CoreClient over the Phase 9 / 9c HTTP API.
// =============================================================================
#ifndef NEXOR_COMMAND_MAINWINDOW_H
#define NEXOR_COMMAND_MAINWINDOW_H

#include <QMainWindow>
#include "CoreClient.h"

class QTableWidget;
class QLabel;
class QPushButton;
class QPlainTextEdit;
class QComboBox;

namespace nx {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onRefresh();
    void onDeploy();
    void onRollback();
    void onSettings();
    void onPackagesReceived(const QVector<PackageRow> &rows);
    void onOperationFinished(const QString &op, bool ok, const QString &message);
    void onHealthReceived(bool ok, const QString &info);

private:
    void setupUi();
    void buildMenus();
    void loadSettings();      // pull URL+token into client
    void log(const QString &line, const QString &color = QString());
    PackageRow currentRow() const;

    CoreClient    *m_client;
    QLabel        *m_titleLabel;
    QLabel        *m_healthDot;
    QComboBox     *m_filterCombo;
    QPushButton   *m_refreshBtn;
    QPushButton   *m_deployBtn;
    QPushButton   *m_rollbackBtn;
    QPushButton   *m_settingsBtn;
    QTableWidget  *m_table;
    QPlainTextEdit *m_log;

    QVector<PackageRow> m_rows;
};

} // namespace nx

#endif // NEXOR_COMMAND_MAINWINDOW_H
