// =============================================================================
// NewActivityDialog — collects metadata for a new Atomic Activity.
// =============================================================================
#ifndef NEXOR_STUDIO_NEWACTIVITYDIALOG_H
#define NEXOR_STUDIO_NEWACTIVITYDIALOG_H

#include <QDialog>
#include "project/Activity.h"

class QLineEdit;
class QPlainTextEdit;
class QDateTimeEdit;
class QPushButton;

class NewActivityDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewActivityDialog(QWidget *parent = nullptr);

    ActivityMeta meta() const;

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

#endif // NEXOR_STUDIO_NEWACTIVITYDIALOG_H
