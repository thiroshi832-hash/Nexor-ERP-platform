// =============================================================================
// SettingsDialog — edits the Core URL + admin token Command uses to talk to
// Nexor Core.  Same shape as Studio's Publishing Settings dialog so the same
// values work on both ends.
// =============================================================================
#ifndef NEXOR_COMMAND_SETTINGSDIALOG_H
#define NEXOR_COMMAND_SETTINGSDIALOG_H

#include <QDialog>

class QLineEdit;

namespace nx {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    QString coreUrl()    const;
    QString adminToken() const;

    void setCoreUrl   (const QString &v);
    void setAdminToken(const QString &v);

private:
    QLineEdit *m_urlEdit;
    QLineEdit *m_tokenEdit;
};

} // namespace nx

#endif // NEXOR_COMMAND_SETTINGSDIALOG_H
