// =============================================================================
// SettingsDialog — Core's environment-variable editor.
//
// Edits the four settings the administrator typically wants to control:
//   • Listening port
//   • Data root (where core.db, core_entities.db, core_processes.db, and
//                packages/<id>/<ver>.nexor live)
//   • Signing key (HMAC-SHA256; empty = permissive)
//   • Admin token (bearer; empty = open)
//
// Persisted via QSettings under the "Server/" group so Core remembers
// across sessions.  Command-line flags (--port, --data, --signing-key,
// --admin-token) still override these on startup.
// =============================================================================
#ifndef NEXOR_CORE_SETTINGSDIALOG_H
#define NEXOR_CORE_SETTINGSDIALOG_H

#include <QDialog>

class QLineEdit;
class QSpinBox;
class QPushButton;

namespace nx {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    quint16 port()        const;
    QString dataRoot()    const;
    QString signingKey()  const;
    QString adminToken()  const;

    void setPort       (quint16 v);
    void setDataRoot   (const QString &v);
    void setSigningKey (const QString &v);
    void setAdminToken (const QString &v);

private slots:
    void onBrowseDataRoot();

private:
    QSpinBox  *m_portSpin;
    QLineEdit *m_dataEdit;
    QLineEdit *m_keyEdit;
    QLineEdit *m_tokenEdit;
};

} // namespace nx

#endif // NEXOR_CORE_SETTINGSDIALOG_H
