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
class CodeEditor;

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

private:
    void setupUi();
    void rebuildTable();
    void writeBack();   // table → m_process->steps()  (excluding active step's code)

    QString                  m_path;
    std::unique_ptr<Process> m_process;
    int                      m_activeRow { -1 };

    QLabel        *m_titleLabel;
    QTableWidget  *m_table;
    QPushButton   *m_addBtn;
    QPushButton   *m_removeBtn;
    QPushButton   *m_saveBtn;
    QPushButton   *m_runBtn;
    QLabel        *m_codeHeader;
    CodeEditor    *m_codeEditor;
};

#endif // NEXOR_STUDIO_PROCESSEDITOR_H
