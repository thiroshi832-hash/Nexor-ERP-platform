// =============================================================================
// NewProcessDialog — collects metadata for a new Process activity.
// Mirrors NewSheetDialog / NewActivityDialog shape so users feel at home.
// =============================================================================
#ifndef NEXOR_STUDIO_NEWPROCESSDIALOG_H
#define NEXOR_STUDIO_NEWPROCESSDIALOG_H

#include <QDialog>
#include "project/Process.h"

class QLineEdit;
class QPlainTextEdit;
class QDateTimeEdit;
class QPushButton;

class NewProcessDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewProcessDialog(QWidget *parent = nullptr);

    ProcessMeta meta() const;

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

#endif // NEXOR_STUDIO_NEWPROCESSDIALOG_H
