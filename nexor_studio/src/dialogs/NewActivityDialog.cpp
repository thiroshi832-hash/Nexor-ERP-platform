#include "NewActivityDialog.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

NewActivityDialog::NewActivityDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("New Atomic Activity");
    setModal(true);
    resize(500, 400);

    auto *form = new QFormLayout;
    m_titleEdit  = new QLineEdit;  m_titleEdit->setPlaceholderText("My Activity");
    m_idEdit     = new QLineEdit;  m_idEdit->setPlaceholderText("MyActivity");
    m_descEdit   = new QPlainTextEdit;
    m_descEdit->setPlaceholderText("Optional description");
    m_descEdit->setFixedHeight(70);
    m_authorEdit = new QLineEdit;
    m_createdEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    m_createdEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_createdEdit->setCalendarPopup(true);

    form->addRow("Title:",       m_titleEdit);
    form->addRow("Name (ID):",   m_idEdit);
    form->addRow("Description:", m_descEdit);
    form->addRow("Author:",      m_authorEdit);
    form->addRow("Create date:", m_createdEdit);

    auto *btns = new QHBoxLayout;
    btns->addStretch();
    m_cancelBtn = new QPushButton("Cancel");
    m_okBtn     = new QPushButton("Create");
    m_okBtn->setDefault(true);
    m_okBtn->setEnabled(false);
    btns->addWidget(m_cancelBtn);
    btns->addWidget(m_okBtn);

    auto *root = new QVBoxLayout(this);
    root->addLayout(form);
    root->addStretch();
    root->addLayout(btns);

    connect(m_okBtn,     &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto reval = [this]{ validate(); };
    connect(m_titleEdit, &QLineEdit::textChanged, this, reval);
    connect(m_idEdit,    &QLineEdit::textChanged, this, reval);

    connect(m_titleEdit, &QLineEdit::textEdited, this, [this](const QString &t){
        if (m_idEdit->text().isEmpty() || m_idEdit->property("autosynced").toBool()) {
            QString candidate;
            for (QChar c : t) if (c.isLetterOrNumber()) candidate += c;
            m_idEdit->blockSignals(true);
            m_idEdit->setText(candidate);
            m_idEdit->setProperty("autosynced", true);
            m_idEdit->blockSignals(false);
            validate();
        }
    });
    connect(m_idEdit, &QLineEdit::textEdited, this, [this]{
        m_idEdit->setProperty("autosynced", false);
    });
}

void NewActivityDialog::validate() {
    bool ok = !m_titleEdit->text().trimmed().isEmpty()
           && !m_idEdit->text().trimmed().isEmpty();
    m_okBtn->setEnabled(ok);
}

ActivityMeta NewActivityDialog::meta() const {
    ActivityMeta m;
    m.title       = m_titleEdit->text().trimmed();
    m.id          = m_idEdit->text().trimmed();
    m.description = m_descEdit->toPlainText().trimmed();
    m.author      = m_authorEdit->text().trimmed();
    m.created     = m_createdEdit->dateTime();
    return m;
}
