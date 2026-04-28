// =============================================================================
// SheetEditor — visual schema editor for a Sheet (.sht).
//
//   ┌──────────────────────────────────────────────────┐
//   │ Sheet:  Customer                                 │
//   ├──────────────────────────────────────────────────┤
//   │ Name        Type      Key  Required  Default     │
//   │ Id          Long       ✓     ✓                   │
//   │ Name        String          ✓                    │
//   │ Email       String                               │
//   │ Balance     Decimal                              │
//   │  …                                               │
//   ├──────────────────────────────────────────────────┤
//   │ [ + Add field ] [ × Remove ]      [ Save ]       │
//   └──────────────────────────────────────────────────┘
// =============================================================================
#ifndef NEXOR_STUDIO_SHEETEDITOR_H
#define NEXOR_STUDIO_SHEETEDITOR_H

#include <QWidget>
#include <QString>
#include <memory>

#include "project/Sheet.h"

class QTableWidget;
class QLabel;
class QPushButton;

class SheetEditor : public QWidget {
    Q_OBJECT
public:
    explicit SheetEditor(QWidget *parent = nullptr);

    bool    loadSheet(const QString &filePath);
    bool    saveSheet();
    void    clearSheet();
    QString currentSheetPath() const { return m_path; }

signals:
    void modified();

private slots:
    void onAddField();
    void onRemoveField();
    void onSaveClicked();

private:
    void setupUi();
    void rebuildTable();
    void writeBack();   // copy table → m_sheet->fields()

    QString                 m_path;
    std::unique_ptr<Sheet>  m_sheet;

    QLabel        *m_titleLabel;
    QTableWidget  *m_table;
    QPushButton   *m_addBtn;
    QPushButton   *m_removeBtn;
    QPushButton   *m_saveBtn;
};

#endif // NEXOR_STUDIO_SHEETEDITOR_H
