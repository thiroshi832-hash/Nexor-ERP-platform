#include "FluxWindow.h"
#include "CoreClient.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QDateTime>

FluxWindow::FluxWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_client(new CoreClient(this))
    , m_hostEdit(nullptr)
    , m_portSpin(nullptr)
    , m_connectBtn(nullptr)
    , m_messageEdit(nullptr)
    , m_sendBtn(nullptr)
    , m_log(nullptr)
    , m_status(nullptr) {

    setWindowTitle("Nexor Flux");
    resize(720, 480);
    setupUi();

    connect(m_client, &CoreClient::connected,        this, &FluxWindow::onConnected);
    connect(m_client, &CoreClient::disconnected,     this, &FluxWindow::onDisconnected);
    connect(m_client, &CoreClient::messageReceived,  this, &FluxWindow::onMessage);
    connect(m_client, &CoreClient::errorOccurred,    this, &FluxWindow::onError);
}

FluxWindow::~FluxWindow() = default;

void FluxWindow::setupUi() {
    setStyleSheet(R"(
        QMainWindow, QWidget { background-color:#1b1d23; color:#dce1e7;
                               font-family:"Segoe UI"; font-size:13px; }
        QLineEdit, QSpinBox, QPlainTextEdit {
            background:#262932; border:1px solid #353945; border-radius:5px;
            padding:5px 8px; selection-background-color:#3d7ebf;
        }
        QLineEdit:focus, QSpinBox:focus { border-color:#5b8cff; }
        QPushButton {
            background:#2563eb; color:white; font-weight:600; border:none;
            border-radius:5px; padding:6px 16px;
        }
        QPushButton:hover    { background:#3b82f6; }
        QPushButton:disabled { background:#3a3f4b; color:#7a7f8a; }
        QStatusBar QLabel    { color:#8a95a3; }
    )");

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(12,12,12,12);
    root->setSpacing(8);

    // Connection bar
    auto *connBar = new QHBoxLayout;
    connBar->addWidget(new QLabel("Host:"));
    m_hostEdit = new QLineEdit("127.0.0.1");
    m_hostEdit->setFixedWidth(140);
    connBar->addWidget(m_hostEdit);
    connBar->addWidget(new QLabel("Port:"));
    m_portSpin = new QSpinBox;
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(7421);
    m_portSpin->setFixedWidth(90);
    connBar->addWidget(m_portSpin);
    m_connectBtn = new QPushButton("Connect");
    connBar->addWidget(m_connectBtn);
    connBar->addStretch();
    root->addLayout(connBar);

    // Log
    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setStyleSheet(m_log->styleSheet() +
        "QPlainTextEdit{ font-family:'Consolas','Courier New',monospace; font-size:12px; }");
    root->addWidget(m_log, 1);

    // Send bar
    auto *sendBar = new QHBoxLayout;
    m_messageEdit = new QLineEdit;
    m_messageEdit->setPlaceholderText("Type message and Send (server echoes back).");
    sendBar->addWidget(m_messageEdit, 1);
    m_sendBtn = new QPushButton("Send");
    m_sendBtn->setEnabled(false);
    sendBar->addWidget(m_sendBtn);
    root->addLayout(sendBar);

    m_status = new QLabel("Disconnected");
    statusBar()->addWidget(m_status);

    connect(m_connectBtn, &QPushButton::clicked, this, &FluxWindow::onConnectClicked);
    connect(m_sendBtn,    &QPushButton::clicked, this, &FluxWindow::onSendClicked);
    connect(m_messageEdit,&QLineEdit::returnPressed, this, &FluxWindow::onSendClicked);
}

void FluxWindow::appendLog(const QString &text, const QString &color) {
    QString stamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString c = color.isEmpty() ? "#dce1e7" : color;
    m_log->appendHtml(QString("<span style='color:#6b7280'>[%1]</span> "
                              "<span style='color:%2'>%3</span>")
                        .arg(stamp, c, text.toHtmlEscaped()));
}

void FluxWindow::onConnectClicked() {
    if (m_client->isConnected()) {
        m_client->disconnectFromCore();
    } else {
        appendLog(QString("Connecting to %1:%2 ...")
                    .arg(m_hostEdit->text()).arg(m_portSpin->value()), "#facc15");
        m_client->connectToCore(m_hostEdit->text(), (quint16)m_portSpin->value());
    }
}

void FluxWindow::onSendClicked() {
    QString msg = m_messageEdit->text();
    if (msg.isEmpty() || !m_client->isConnected()) return;
    m_client->send(msg.toUtf8());
    appendLog("→ " + msg, "#7dd3fc");
    m_messageEdit->clear();
}

void FluxWindow::onConnected() {
    m_status->setText("Connected");
    m_connectBtn->setText("Disconnect");
    m_sendBtn->setEnabled(true);
    appendLog("Connected.", "#22c55e");
}

void FluxWindow::onDisconnected() {
    m_status->setText("Disconnected");
    m_connectBtn->setText("Connect");
    m_sendBtn->setEnabled(false);
    appendLog("Disconnected.", "#ef4444");
}

void FluxWindow::onMessage(const QByteArray &data) {
    appendLog("← " + QString::fromUtf8(data), "#a3e635");
}

void FluxWindow::onError(const QString &what) {
    appendLog("Error: " + what, "#ef4444");
}
