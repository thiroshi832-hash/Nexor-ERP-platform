#include "TabOrderDialog.h"
#include "designer/FormCanvas.h"

#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

TabOrderDialog::TabOrderDialog(FormCanvas *canvas, QWidget *parent)
    : QDialog(parent), m_canvas(canvas) {
    setWindowTitle("Tab Order");
    setModal(true);
    resize(360, 420);

    auto *col = new QVBoxLayout(this);
    col->setSpacing(8);

    auto *hint = new QLabel(
        "Reorder widgets by selecting one and clicking Move Up / Down.\n"
        "OK applies the new tab order.", this);
    hint->setStyleSheet("color:#8a95a3;");
    col->addWidget(hint);

    m_list = new QListWidget(this);
    if (m_canvas) {
        int i = 1;
        for (const auto &it : m_canvas->items()) {
            auto *item = new QListWidgetItem(
                QString("%1.  %2   (%3)").arg(i++, 2)
                                        .arg(it.name, it.type));
            item->setData(Qt::UserRole, it.name);
            m_list->addItem(item);
        }
    }
    if (m_list->count() > 0) m_list->setCurrentRow(0);
    col->addWidget(m_list, 1);

    auto *btnRow = new QHBoxLayout;
    auto *upBtn   = new QPushButton("Move Up",   this);
    auto *downBtn = new QPushButton("Move Down", this);
    btnRow->addWidget(upBtn);
    btnRow->addWidget(downBtn);
    btnRow->addStretch();
    auto *cancel  = new QPushButton("Cancel", this);
    auto *ok      = new QPushButton("OK",     this);
    ok->setDefault(true);
    btnRow->addWidget(cancel);
    btnRow->addWidget(ok);
    col->addLayout(btnRow);

    connect(upBtn,   &QPushButton::clicked, this, &TabOrderDialog::onUp);
    connect(downBtn, &QPushButton::clicked, this, &TabOrderDialog::onDown);
    connect(cancel,  &QPushButton::clicked, this, &QDialog::reject);
    connect(ok,      &QPushButton::clicked, this, &TabOrderDialog::onAccept);
}

void TabOrderDialog::onUp() {
    int r = m_list->currentRow();
    if (r <= 0) return;
    auto *item = m_list->takeItem(r);
    m_list->insertItem(r - 1, item);
    m_list->setCurrentRow(r - 1);
    // Re-number visible labels
    for (int i = 0; i < m_list->count(); ++i) {
        auto *it = m_list->item(i);
        QString name = it->data(Qt::UserRole).toString();
        QString suffix = it->text().section('(', 1).prepend('(');
        it->setText(QString("%1.  %2   %3").arg(i+1, 2).arg(name, suffix));
    }
}

void TabOrderDialog::onDown() {
    int r = m_list->currentRow();
    if (r < 0 || r >= m_list->count() - 1) return;
    auto *item = m_list->takeItem(r);
    m_list->insertItem(r + 1, item);
    m_list->setCurrentRow(r + 1);
    for (int i = 0; i < m_list->count(); ++i) {
        auto *it = m_list->item(i);
        QString name = it->data(Qt::UserRole).toString();
        QString suffix = it->text().section('(', 1).prepend('(');
        it->setText(QString("%1.  %2   %3").arg(i+1, 2).arg(name, suffix));
    }
}

void TabOrderDialog::onAccept() {
    QStringList order;
    for (int i = 0; i < m_list->count(); ++i)
        order << m_list->item(i)->data(Qt::UserRole).toString();
    if (m_canvas) m_canvas->reorderItems(order);
    accept();
}
