#include "SettingsDialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>

namespace nx {

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Connection Settings");
    setModal(true);
    resize(440, 0);

    auto *form = new QFormLayout;
    m_urlEdit   = new QLineEdit;
    m_tokenEdit = new QLineEdit;
    m_urlEdit  ->setPlaceholderText("http://localhost:7421");
    m_tokenEdit->setPlaceholderText("(blank for permissive Core)");
    form->addRow("Core URL:",    m_urlEdit);
    form->addRow("Admin token:", m_tokenEdit);

    auto *bb = new QHBoxLayout;
    auto *cancel = new QPushButton("Cancel");
    auto *save   = new QPushButton("Save");
    save->setDefault(true);
    bb->addStretch(); bb->addWidget(cancel); bb->addWidget(save);

    auto *root = new QVBoxLayout(this);
    root->addLayout(form);
    root->addLayout(bb);

    connect(save,   &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
}

QString SettingsDialog::coreUrl()    const { return m_urlEdit  ->text().trimmed(); }
QString SettingsDialog::adminToken() const { return m_tokenEdit->text();           }

void SettingsDialog::setCoreUrl   (const QString &v) { m_urlEdit  ->setText(v); }
void SettingsDialog::setAdminToken(const QString &v) { m_tokenEdit->setText(v); }

} // namespace nx
