// =============================================================================
// ActivityPicker — modal dialog that lists every form belonging to every
// activity in an installed package, and runs the selected form via the
// existing FormRunner so what Studio's designer drew is exactly what Flux
// shows the user.
// =============================================================================
#ifndef NEXOR_FLUX_ACTIVITYPICKER_H
#define NEXOR_FLUX_ACTIVITYPICKER_H

#include <QDialog>
#include <memory>

class Project;
class QTreeWidget;
class QPushButton;
class QPlainTextEdit;
namespace nx { class CoreClient; }

namespace nx {

class ActivityPicker : public QDialog {
    Q_OBJECT
public:
    // `projectFile` points at a .pro inside the Flux extracted/ tree.
    // `coreUrl` and `adminToken` (optional) are forwarded to the RPC bridge
    // when the package contains [Activity(RunsOn := ServerOnly)] subs.
    // `packageId` is the live package's id on Core (usually the project id).
    explicit ActivityPicker(const QString &projectFile,
                            const QString &coreUrl,
                            const QString &adminToken,
                            const QString &packageId,
                            QWidget *parent = nullptr);
    ~ActivityPicker() override;

private slots:
    void onRunForm();
    void onRunActivityMain();
    void onRunProcess();

private:
    void populateTree();
    void appendOutput(const QString &line, const QString &color = QString());

    QString                   m_projectFile;
    QString                   m_coreUrl;
    QString                   m_adminToken;
    QString                   m_packageId;
    std::unique_ptr<Project>  m_project;

    QTreeWidget   *m_tree;
    QPushButton   *m_runFormBtn;
    QPushButton   *m_runMainBtn;
    QPushButton   *m_runProcBtn;
    QPlainTextEdit *m_log;
};

} // namespace nx

#endif // NEXOR_FLUX_ACTIVITYPICKER_H
