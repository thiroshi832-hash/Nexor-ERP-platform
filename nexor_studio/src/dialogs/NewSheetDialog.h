// =============================================================================
// NewSheetDialog — collects metadata for a new Sheet (entity definition).
// Identical shape to NewActivityDialog (title / id / description / author /
// created), so users feel at home.
// =============================================================================
#ifndef NEXOR_STUDIO_NEWSHEETDIALOG_H
#define NEXOR_STUDIO_NEWSHEETDIALOG_H

#include <QDialog>
#include "project/Sheet.h"

class QLineEdit;
class QPlainTextEdit;
class QDateTimeEdit;
class QPushButton;

class NewSheetDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewSheetDialog(QWidget *parent = nullptr);

    SheetMeta meta() const;

private slots:
    void validate();

private:
    QLineEdit      *m_titleEdit;
    QLineEdit      *m_idEdit;
    QPlainTextEdit *m_descEdit;
    QLineEdit      *m_authorEdit;
    QDateTimeEdit  *m_createdEdit;
    QPushButton    *m_okBtn;
    QPushButton    *m_cancelBtn;
};

#endif // NEXOR_STUDIO_NEWSHEETDIALOG_H
