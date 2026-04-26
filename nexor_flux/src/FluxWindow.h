// =============================================================================
// FluxWindow — Nexor Flux client main window.
// =============================================================================
#ifndef NEXOR_FLUX_FLUXWINDOW_H
#define NEXOR_FLUX_FLUXWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QSpinBox;
class QPushButton;
class QPlainTextEdit;
class QLabel;
class CoreClient;

class FluxWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit FluxWindow(QWidget *parent = nullptr);
    ~FluxWindow() override;

private slots:
    void onConnectClicked();
    void onSendClicked();
    void onConnected();
    void onDisconnected();
    void onMessage(const QByteArray &data);
    void onError(const QString &what);

private:
    void setupUi();
    void appendLog(const QString &text, const QString &color = QString());

    CoreClient     *m_client;
    QLineEdit      *m_hostEdit;
    QSpinBox       *m_portSpin;
    QPushButton    *m_connectBtn;
    QLineEdit      *m_messageEdit;
    QPushButton    *m_sendBtn;
    QPlainTextEdit *m_log;
    QLabel         *m_status;
};

#endif // NEXOR_FLUX_FLUXWINDOW_H
