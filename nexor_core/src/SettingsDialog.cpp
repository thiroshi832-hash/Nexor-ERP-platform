#include "SettingsDialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>

namespace nx {

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Server Settings");
    setModal(true);
    resize(560, 0);

    auto *form = new QFormLayout;

    m_portSpin = new QSpinBox;
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(7421);

    m_dataEdit = new QLineEdit;
    m_dataEdit->setPlaceholderText("(default: %APPDATA%/Nexor/Core)");
    m_dataEdit->setMinimumWidth(360);
    auto *browseBtn = new QPushButton("Browse...");
    auto *dataRow = new QWidget;
    auto *dataLay = new QHBoxLayout(dataRow);
    dataLay->setContentsMargins(0, 0, 0, 0);
    dataLay->addWidget(m_dataEdit, 1);
    dataLay->addWidget(browseBtn);

    m_keyEdit   = new QLineEdit;
    m_keyEdit  ->setPlaceholderText("(blank = packages can be unsigned)");
    m_tokenEdit = new QLineEdit;
    m_tokenEdit->setPlaceholderText("(blank = no auth on POST + /admin/*)");

    form->addRow("Listening port:",  m_portSpin);
    form->addRow("Data root:",       dataRow);
    form->addRow("Signing key:",     m_keyEdit);
    form->addRow("Admin token:",     m_tokenEdit);

    auto *hint = new QLabel(
        "<p style='color:#8a95a3'>Changes take effect on the next server start. "
        "Use <b>Server → Restart</b> after editing.</p>");
    hint->setWordWrap(true);
    form->addRow(hint);

    auto *bb = new QHBoxLayout;
    auto *cancel = new QPushButton("Cancel");
    auto *save   = new QPushButton("Save");
    save->setDefault(true);
    bb->addStretch(); bb->addWidget(cancel); bb->addWidget(save);

    auto *root = new QVBoxLayout(this);
    root->addLayout(form);
    root->addLayout(bb);

    connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseDataRoot);
    connect(save,      &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel,    &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::onBrowseDataRoot() {
    QString d = QFileDialog::getExistingDirectory(this, "Pick data root",
        m_dataEdit->text());
    if (!d.isEmpty()) m_dataEdit->setText(d);
}

quint16 SettingsDialog::port()       const { return static_cast<quint16>(m_portSpin->value()); }
QString SettingsDialog::dataRoot()   const { return m_dataEdit  ->text().trimmed(); }
QString SettingsDialog::signingKey() const { return m_keyEdit   ->text(); }
QString SettingsDialog::adminToken() const { return m_tokenEdit ->text(); }

void SettingsDialog::setPort       (quint16 v)         { m_portSpin->setValue(v); }
void SettingsDialog::setDataRoot   (const QString &v)  { m_dataEdit  ->setText(v); }
void SettingsDialog::setSigningKey (const QString &v)  { m_keyEdit   ->setText(v); }
void SettingsDialog::setAdminToken (const QString &v)  { m_tokenEdit ->setText(v); }

} // namespace nx
