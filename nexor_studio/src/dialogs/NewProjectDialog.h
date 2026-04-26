// =============================================================================
// NewProjectDialog — collects project metadata + parent directory.
// =============================================================================
#ifndef NEXOR_STUDIO_NEWPROJECTDIALOG_H
#define NEXOR_STUDIO_NEWPROJECTDIALOG_H

#include <QDialog>
#include "project/Project.h"

class QLineEdit;
class QPlainTextEdit;
class QDateTimeEdit;
class QPushButton;

class NewProjectDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewProjectDialog(QWidget *parent = nullptr);

    ProjectMeta meta() const;
    QString     parentDirectory() const;   // dir the project folder is created under

private slots:
    void onBrowse();
    void validate();

private:
    QLineEdit      *m_titleEdit;
    QLineEdit      *m_idEdit;
    QPlainTextEdit *m_descEdit;
    QLineEdit      *m_authorEdit;
    QDateTimeEdit  *m_createdEdit;
    QLineEdit      *m_dirEdit;
    QPushButton    *m_browseBtn;
    QPushButton    *m_okBtn;
    QPushButton    *m_cancelBtn;
};

#endif // NEXOR_STUDIO_NEWPROJECTDIALOG_H
