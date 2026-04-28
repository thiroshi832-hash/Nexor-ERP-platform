// =============================================================================
// ProcessEditor — visual editor for a Process activity (.prc).
//
//   ┌──────────────────────────────────────────────────────────────────┐
//   │ Process:  OrderApproval                          [ Run Process ] │
//   ├──────────────────────────────────────────────────────────────────┤
//   │ ID            Type     Next                                      │
//   │ Start         Server   Approve                                   │
//   │ Approve       Server   End                                       │
//   │ End           Final                                              │
//   ├──────────────────────────────────────────────────────────────────┤
//   │ [ + Add step ]  [ × Remove ]                       [ Save ]      │
//   ├──────────────────────────────────────────────────────────────────┤
//   │ Step Code (Approve):                                             │
//   │ ┌──────────────────────────────────────────────────────────────┐ │
//   │ │   Print "Approving..."                                       │ │
//   │ │                                                              │ │
//   │ └──────────────────────────────────────────────────────────────┘ │
//   └──────────────────────────────────────────────────────────────────┘
// =============================================================================
#ifndef NEXOR_STUDIO_PROCESSEDITOR_H
#define NEXOR_STUDIO_PROCESSEDITOR_H

#include <QWidget>
#include <QString>
#include <memory>

#include "project/Process.h"

class QTableWidget;
class QLabel;
class QPushButton;
class QStackedWidget;
class QComboBox;
class CodeEditor;
namespace nx { class BpmnCanvas; }

class ProcessEditor : public QWidget {
    Q_OBJECT
public:
    explicit ProcessEditor(QWidget *parent = nullptr);

    bool    loadProcess(const QString &filePath);
    bool    saveProcess();
    void    clearProcess();
    QString currentProcessPath() const { return m_path; }

signals:
    void modified();
    void runRequested(const QString &absoluteProcessPath);

private slots:
    void onAddStep();
    void onRemoveStep();
    void onSaveClicked();
    void onRunClicked();
    void onRowChanged(int row);
    void onCodeChanged();
    void onViewModeChanged(int idx);
    void onBpmnSelectionChanged(const QString &stepId);
    void onBpmnModelChanged();
    void onExportBpmn();
    void onImportBpmn();

private:
    void setupUi();
    void rebuildTable();
    void writeBack();   // table → m_process->steps()  (excluding active step's code)

    QString                  m_path;
    std::unique_ptr<Process> m_process;
    int                      m_activeRow { -1 };

    QLabel        *m_titleLabel;
    QComboBox     *m_viewCombo;       // Diagram / Table
    QStackedWidget *m_viewStack;
    nx::BpmnCanvas *m_canvas;
    QTableWidget  *m_table;
    QPushButton   *m_addBtn;
    QPushButton   *m_removeBtn;
    QPushButton   *m_saveBtn;
    QPushButton   *m_runBtn;
    QPushButton   *m_exportBtn;
    QPushButton   *m_importBtn;
    QLabel        *m_codeHeader;
    CodeEditor    *m_codeEditor;
};

#endif // NEXOR_STUDIO_PROCESSEDITOR_H
