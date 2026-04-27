#include "PropertyPanel.h"
#include "FormCanvas.h"
#include "WidgetFactory.h"

#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>

PropertyPanel::PropertyPanel(QWidget *parent) : QWidget(parent) {
    setObjectName("propertyPanel");
    setStyleSheet(R"(
        #propertyPanel { background:#1b1d23; border-left:1px solid #1e2030; }
        QLabel#paneHeader {
            background:#0d0e12; color:#8a95a3;
            padding:8px 12px; border-bottom:1px solid #1e2030;
            font-size:11px; font-weight:600; letter-spacing:2px;
        }
        QLabel#fieldLabel { color:#8a95a3; font-size:11px; }
        QLabel#typeLabel  { color:#5b8cff; font-size:13px; font-weight:600; padding:2px 0; }
        QLabel#emptyLabel { color:#6a6a6a; font-size:11px; padding:24px 16px; }
        QLineEdit, QSpinBox {
            background:#262932; color:#dce1e7;
            border:1px solid #353945; border-radius:3px;
            padding:3px 6px; font-size:12px;
        }
        QLineEdit:focus, QSpinBox:focus { border-color:#5b8cff; }
    )");

    auto *col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    auto *header = new QLabel("PROPERTIES", this);
    header->setObjectName("paneHeader");
    col->addWidget(header);

    // ── Form layout for editable fields ─────────────────────────
    auto *body = new QWidget(this);
    auto *bodyCol = new QVBoxLayout(body);
    bodyCol->setContentsMargins(12, 12, 12, 12);
    bodyCol->setSpacing(8);

    m_typeLabel = new QLabel(body);
    m_typeLabel->setObjectName("typeLabel");
    bodyCol->addWidget(m_typeLabel);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignLeft);
    form->setHorizontalSpacing(10);
    form->setVerticalSpacing(6);

    auto fieldLabel = [&](const QString &t) {
        auto *l = new QLabel(t, body);
        l->setObjectName("fieldLabel");
        return l;
    };

    m_nameEdit = new QLineEdit(body);
    form->addRow(fieldLabel("Name"), m_nameEdit);

    auto rowSpins = [&](const QString &t1, QSpinBox *&s1,
                        const QString &t2, QSpinBox *&s2) {
        s1 = new QSpinBox(body); s1->setRange(0, 4000); s1->setFixedWidth(70);
        s2 = new QSpinBox(body); s2->setRange(0, 4000); s2->setFixedWidth(70);
        auto *row = new QWidget(body);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0); h->setSpacing(6);
        h->addWidget(new QLabel(t1));
        h->addWidget(s1);
        h->addSpacing(8);
        h->addWidget(new QLabel(t2));
        h->addWidget(s2);
        h->addStretch();
        return row;
    };
    QSpinBox *xS=nullptr,*yS=nullptr,*wS=nullptr,*hS=nullptr;
    auto *posRow = rowSpins("x", xS, "y", yS);
    auto *sizeRow = rowSpins("w", wS, "h", hS);
    m_xSpin = xS; m_ySpin = yS; m_wSpin = wS; m_hSpin = hS;
    form->addRow(fieldLabel("Position"), posRow);
    form->addRow(fieldLabel("Size"),     sizeRow);

    m_textEdit = new QLineEdit(body);
    form->addRow(fieldLabel("Text"), m_textEdit);

    bodyCol->addLayout(form);
    bodyCol->addStretch();

    m_emptyLabel = new QLabel(
        "Select a widget on the canvas to edit its properties.", body);
    m_emptyLabel->setObjectName("emptyLabel");
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    bodyCol->addWidget(m_emptyLabel);

    col->addWidget(body, 1);

    // Wire field edits → canvas mutators
    connect(m_nameEdit, &QLineEdit::editingFinished, this, &PropertyPanel::onNameEdited);
    connect(m_textEdit, &QLineEdit::textEdited,      this, &PropertyPanel::onTextEdited);
    auto bumpGeo = [this]{ onGeometryEdited(); };
    connect(m_xSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, bumpGeo);
    connect(m_ySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, bumpGeo);
    connect(m_wSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, bumpGeo);
    connect(m_hSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, bumpGeo);

    setEnabledFields(false);
}

void PropertyPanel::setCanvas(FormCanvas *canvas) {
    if (m_canvas) disconnect(m_canvas, nullptr, this, nullptr);
    m_canvas = canvas;
    if (m_canvas) {
        connect(m_canvas, &FormCanvas::selectionChanged,
                this, &PropertyPanel::onSelectionChanged);
        connect(m_canvas, &FormCanvas::modified,
                this, &PropertyPanel::refreshFromSelection);
    }
    onSelectionChanged(m_canvas ? m_canvas->selectedWidget() : nullptr);
}

void PropertyPanel::setEnabledFields(bool on) {
    m_typeLabel->setVisible(on);
    m_nameEdit->setEnabled(on);
    m_xSpin->setEnabled(on);
    m_ySpin->setEnabled(on);
    m_wSpin->setEnabled(on);
    m_hSpin->setEnabled(on);
    m_textEdit->setEnabled(on);
    m_emptyLabel->setVisible(!on);
}

void PropertyPanel::onSelectionChanged(QWidget *w) {
    if (!w) {
        setEnabledFields(false);
        m_typeLabel->clear();
        m_nameEdit->clear();
        m_textEdit->clear();
        m_xSpin->setValue(0); m_ySpin->setValue(0);
        m_wSpin->setValue(0); m_hSpin->setValue(0);
        return;
    }
    setEnabledFields(true);
    refreshFromSelection();
}

void PropertyPanel::refreshFromSelection() {
    if (!m_canvas) return;
    QWidget *w = m_canvas->selectedWidget();
    if (!w) return;
    m_updating = true;
    m_typeLabel->setText(m_canvas->selectedType());
    m_nameEdit->setText(m_canvas->selectedName());
    m_xSpin->setValue(w->x());
    m_ySpin->setValue(w->y());
    m_wSpin->setValue(w->width());
    m_hSpin->setValue(w->height());
    if (WidgetFactory::hasTextProperty(m_canvas->selectedType())) {
        m_textEdit->setEnabled(true);
        m_textEdit->setText(WidgetFactory::readProperty(w, "text").toString());
    } else {
        m_textEdit->setEnabled(false);
        m_textEdit->setText("(n/a)");
    }
    m_updating = false;
}

void PropertyPanel::onNameEdited() {
    if (m_updating || !m_canvas) return;
    m_canvas->setNameForSelected(m_nameEdit->text().trimmed());
}

void PropertyPanel::onTextEdited() {
    if (m_updating || !m_canvas) return;
    m_canvas->setTextForSelected(m_textEdit->text());
}

void PropertyPanel::onGeometryEdited() {
    if (m_updating || !m_canvas) return;
    QRect g(m_xSpin->value(), m_ySpin->value(),
            m_wSpin->value(), m_hSpin->value());
    m_canvas->setGeometryForSelected(g);
}
