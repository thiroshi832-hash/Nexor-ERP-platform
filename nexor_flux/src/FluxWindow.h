// =============================================================================
// FluxWindow — Nexor Flux's catalog + run console.
//
//   ┌────────────────────────────────────────────────────────────────────┐
//   │ NEXOR FLUX  —  Core: http://localhost:7421         [● live]        │
//   ├────────────────────────────────────────────────────────────────────┤
//   │ INSTALLED                                                          │
//   │  Sales 0.4.0   live      [Run] [Update→0.4.1] [Uninstall]         │
//   │  Inventory 1.0           [Run] [Uninstall]                         │
//   ├────────────────────────────────────────────────────────────────────┤
//   │ AVAILABLE                                                          │
//   │  Sales 0.4.1   pending  [Install]                                  │
//   │  HR 0.2.0      live     [Install]                                  │
//   ├────────────────────────────────────────────────────────────────────┤
//   │ 12:34 → catalog…                                                   │
//   │ 12:34 ← 4 row(s)                                                   │
//   └────────────────────────────────────────────────────────────────────┘
// =============================================================================
#ifndef NEXOR_FLUX_FLUXWINDOW_H
#define NEXOR_FLUX_FLUXWINDOW_H

#include <QMainWindow>
#include <QHash>
#include <memory>

#include "CoreClient.h"
#include "PackageCache.h"

class QLabel;
class QPushButton;
class QTableWidget;
class QPlainTextEdit;

namespace nx {

class FluxWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit FluxWindow(QWidget *parent = nullptr);
    ~FluxWindow() override;

private slots:
    void onRefresh();
    void onSettings();
    void onInstall();
    void onUninstall();
    void onRun();

    void onCatalogReceived (const QVector<CatalogRow> &rows);
    void onHealthReceived  (bool ok, const QString &info);
    void onDownloadFinished(const QString &id, const QString &version,
                            bool ok, const QString &localPath, const QString &message);

private:
    enum CellRole {
        RoleId   = Qt::UserRole + 1,
        RoleVer  = Qt::UserRole + 2,
        RoleSrc  = Qt::UserRole + 3,    // 0 = installed, 1 = available
    };

    void setupUi();
    void buildMenus();
    void log(const QString &line, const QString &color = QString());
    void rebuildTable();
    void loadSettings();

    CoreClient      *m_client;
    std::unique_ptr<PackageCache> m_cache;

    QLabel          *m_title;
    QLabel          *m_health;
    QTableWidget    *m_table;
    QPushButton     *m_refreshBtn;
    QPushButton     *m_runBtn;
    QPushButton     *m_installBtn;
    QPushButton     *m_uninstallBtn;
    QPlainTextEdit  *m_log;

    // Latest data from Core / cache.
    QVector<CatalogRow>   m_catalog;
    QVector<InstalledRow> m_installed;
};

} // namespace nx

#endif // NEXOR_FLUX_FLUXWINDOW_H
