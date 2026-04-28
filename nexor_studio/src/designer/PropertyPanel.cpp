#include "PropertyPanel.h"
#include "FormCanvas.h"
#include "WidgetFactory.h"

#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QColorDialog>
#include <QFileInfo>

PropertyPanel::PropertyPanel(QWidget *parent) : QWidget(parent) {
    setObjectName("propertyPanel");
    setStyleSheet(R"(
        #propertyPanel { background:#1b1d23; border-left:1px solid #1e2030; }
        QLabel#paneHeader {
            background:#0d0e12; color:#8a95a3;
            padding:8px 12px; border-bottom:1px solid #1e2030;
            font-size:11px; font-weight:600; letter-spacing:2px;
        }
        QLabel#sectionHdr {
            color:#8a95a3; font-size:10px; font-weight:600;
            letter-spacing:2px; padding:10px 0 4px 0;
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

        QToolButton#anchorBtn {
            background:#262932; color:#8a95a3;
            border:1px solid #353945; border-radius:3px;
            min-width:24px; min-height:22px;
            font-weight:600; font-size:11px;
        }
        QToolButton#anchorBtn:checked {
            background:#1e3a5f; color:#dce1e7; border-color:#5b8cff;
        }

        QPushButton#evtBtn {
            background:#262932; color:#a3e635;
            border:1px solid #353945; border-radius:3px;
            padding:4px 8px; font-size:11px; text-align:left;
        }
        QPushButton#evtBtn:hover { background:#2d3140; border-color:#5b8cff; }
    )");

    auto *col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    auto *header = new QLabel("PROPERTIES", this);
    header->setObjectName("paneHeader");
    col->addWidget(header);

    // ── View toggle (Categorized / Alphabetical) ─────────────────────
    auto *toggleRow = new QWidget(this);
    toggleRow->setStyleSheet("background:#181a22; border-bottom:1px solid #1e2030;");
    auto *toggleH = new QHBoxLayout(toggleRow);
    toggleH->setContentsMargins(8, 4, 8, 4); toggleH->setSpacing(4);
    auto makeViewBtn = [&](const QString &t) {
        auto *b = new QToolButton(toggleRow);
        b->setText(t); b->setCheckable(true);
        b->setStyleSheet(
            "QToolButton { background:transparent; color:#8a95a3;"
            " border:none; padding:4px 10px; font-size:11px; font-weight:600;"
            " letter-spacing:1px; }"
            "QToolButton:hover { color:#dce1e7; }"
            "QToolButton:checked { color:#5b8cff; border-bottom:2px solid #5b8cff; }");
        return b;
    };
    m_btnCategorized  = makeViewBtn("Categorized");
    m_btnAlphabetical = makeViewBtn("Alphabetical");
    m_btnCategorized->setChecked(true);
    toggleH->addWidget(m_btnCategorized);
    toggleH->addWidget(m_btnAlphabetical);
    toggleH->addStretch();
    col->addWidget(toggleRow);
    connect(m_btnCategorized,  &QToolButton::clicked, this, [this]{
        m_btnCategorized->setChecked(true);
        m_btnAlphabetical->setChecked(false);
        setView(ViewCategorized);
    });
    connect(m_btnAlphabetical, &QToolButton::clicked, this, [this]{
        m_btnAlphabetical->setChecked(true);
        m_btnCategorized->setChecked(false);
        setView(ViewAlphabetical);
    });

    // ── Body container ─────────────────────────────────────────────────
    auto *body = new QWidget(this);
    auto *bodyCol = new QVBoxLayout(body);
    bodyCol->setContentsMargins(12, 12, 12, 12);
    bodyCol->setSpacing(8);

    m_typeLabel = new QLabel(body);
    m_typeLabel->setObjectName("typeLabel");
    bodyCol->addWidget(m_typeLabel);

    m_form = new QFormLayout;
    m_form->setLabelAlignment(Qt::AlignLeft);
    m_form->setHorizontalSpacing(10);
    m_form->setVerticalSpacing(4);

    auto fieldLabel = [&](const QString &t) {
        auto *l = new QLabel(t, body);
        l->setObjectName("fieldLabel");
        return l;
    };

    // ── Field widgets (kept as members; rebuilt into the layout on toggle) ─
    m_nameEdit = new QLineEdit(body);
    m_textLabel = fieldLabel("Text");
    m_textEdit  = new QLineEdit(body);

    m_xSpin = new QSpinBox(body); m_xSpin->setRange(0, 4000); m_xSpin->setFixedWidth(70);
    m_ySpin = new QSpinBox(body); m_ySpin->setRange(0, 4000); m_ySpin->setFixedWidth(70);
    m_wSpin = new QSpinBox(body); m_wSpin->setRange(0, 4000); m_wSpin->setFixedWidth(70);
    m_hSpin = new QSpinBox(body); m_hSpin->setRange(0, 4000); m_hSpin->setFixedWidth(70);

    m_fgBtn = makeColorBtn();
    m_bgBtn = makeColorBtn();

    m_visibleCheck = new QCheckBox(body);
    m_visibleCheck->setChecked(true);
    {
        auto *row = new QWidget(body);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(0,0,0,0);
        h->addWidget(m_visibleCheck);
        h->addStretch();
        m_visibleRow = row;
    }

    m_anchorT = makeAnchorBtn("T");
    m_anchorL = makeAnchorBtn("L");
    m_anchorR = makeAnchorBtn("R");
    m_anchorB = makeAnchorBtn("B");
    {
        auto *row = new QWidget(body);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(0,0,0,0); h->setSpacing(4);
        h->addWidget(m_anchorT);
        h->addWidget(m_anchorL);
        h->addWidget(m_anchorR);
        h->addWidget(m_anchorB);
        h->addStretch();
        m_anchorRow = row;
    }

    bodyCol->addLayout(m_form);

    // ── Events section ─────────────────────────────────────────────────
    auto *evtHdr = new QLabel("EVENTS", body);
    evtHdr->setObjectName("sectionHdr");
    bodyCol->addWidget(evtHdr);

    m_eventsBox = new QWidget(body);
    m_eventsLayout = new QVBoxLayout(m_eventsBox);
    m_eventsLayout->setContentsMargins(0, 0, 0, 0);
    m_eventsLayout->setSpacing(4);
    bodyCol->addWidget(m_eventsBox);

    bodyCol->addStretch();

    m_emptyLabel = new QLabel(
        "Click the form body to edit form properties,\n"
        "or click a widget to edit its properties.", body);
    m_emptyLabel->setObjectName("emptyLabel");
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    bodyCol->addWidget(m_emptyLabel);

    col->addWidget(body, 1);

    // ── Wire field edits → canvas mutators ────────────────────────────
    connect(m_nameEdit, &QLineEdit::editingFinished, this, &PropertyPanel::onNameEdited);
    connect(m_textEdit, &QLineEdit::textEdited,      this, [this]{
        if (m_mode == ModeForm) onTitleEdited(); else onTextEdited();
    });
    auto bumpGeo = [this]{ onGeometryEdited(); };
    connect(m_xSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, bumpGeo);
    connect(m_ySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, bumpGeo);
    connect(m_wSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, bumpGeo);
    connect(m_hSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, bumpGeo);
    connect(m_fgBtn, &QPushButton::clicked, this, &PropertyPanel::onFgClicked);
    connect(m_bgBtn, &QPushButton::clicked, this, &PropertyPanel::onBgClicked);
    connect(m_visibleCheck, &QCheckBox::toggled, this, &PropertyPanel::onVisibleToggled);
    auto bumpAnchor = [this]{ onAnchorToggled(); };
    connect(m_anchorT, &QToolButton::toggled, this, bumpAnchor);
    connect(m_anchorL, &QToolButton::toggled, this, bumpAnchor);
    connect(m_anchorR, &QToolButton::toggled, this, bumpAnchor);
    connect(m_anchorB, &QToolButton::toggled, this, bumpAnchor);

    setMode(ModeEmpty);
}

QPushButton *PropertyPanel::makeColorBtn() {
    auto *b = new QPushButton(this);
    b->setFixedHeight(22);
    b->setFixedWidth(120);
    b->setCursor(Qt::PointingHandCursor);
    updateColorBtn(b, QColor());
    return b;
}

QToolButton *PropertyPanel::makeAnchorBtn(const QString &letter) {
    auto *b = new QToolButton(this);
    b->setObjectName("anchorBtn");
    b->setText(letter);
    b->setCheckable(true);
    return b;
}

void PropertyPanel::updateColorBtn(QPushButton *btn, const QColor &c) {
    if (c.isValid()) {
        QString fg = (c.lightness() > 130) ? "#000" : "#fff";
        btn->setStyleSheet(QString(
            "background:%1; color:%2; border:1px solid #555; padding:0 6px;")
            .arg(c.name(), fg));
        btn->setText(c.name().toUpper());
    } else {
        btn->setStyleSheet(
            "background:#262932; color:#6a6a6a; border:1px solid #353945; padding:0 6px;");
        btn->setText("(default)");
    }
}

void PropertyPanel::setCanvas(FormCanvas *canvas) {
    if (m_canvas) disconnect(m_canvas, nullptr, this, nullptr);
    m_canvas = canvas;
    if (m_canvas) {
        connect(m_canvas, &FormCanvas::selectionChanged,
                this, &PropertyPanel::onSelectionChanged);
        connect(m_canvas, &FormCanvas::formSelected,
                this, &PropertyPanel::onFormSelected);
        connect(m_canvas, &FormCanvas::modified,
                this, &PropertyPanel::refreshFromSelection);
    }
    onSelectionChanged(m_canvas ? m_canvas->selectedWidget() : nullptr);
}

void PropertyPanel::setMode(Mode m) {
    m_mode = m;
    bool hasSel = (m != ModeEmpty);
    m_typeLabel  ->setVisible(hasSel);
    m_nameEdit   ->setEnabled(hasSel);
    m_xSpin->setEnabled(hasSel); m_ySpin->setEnabled(hasSel);
    m_wSpin->setEnabled(hasSel); m_hSpin->setEnabled(hasSel);
    m_textEdit   ->setEnabled(hasSel);
    m_fgBtn      ->setEnabled(hasSel);
    m_bgBtn      ->setEnabled(hasSel);
    m_visibleRow ->setVisible(m == ModeWidget);
    m_anchorRow  ->setVisible(m == ModeWidget);
    m_emptyLabel ->setVisible(!hasSel);
    m_eventsBox  ->setVisible(hasSel);

    // For the form, X/Y are not meaningful — only W/H.
    m_xSpin->setEnabled(m == ModeWidget);
    m_ySpin->setEnabled(m == ModeWidget);

    // Text / Title label
    m_textLabel->setText(m == ModeForm ? "Title" : "Text");

    rebuildFormLayout();
}

void PropertyPanel::setView(View v) {
    m_view = v;
    rebuildFormLayout();
}

void PropertyPanel::rebuildFormLayout() {
    if (!m_form) return;
    // Detach all rows without deleting their widgets.
    while (m_form->rowCount() > 0) {
        QFormLayout::TakeRowResult r = m_form->takeRow(0);
        if (r.labelItem)  delete r.labelItem;
        if (r.fieldItem)  delete r.fieldItem;
    }
    if (m_mode == ModeEmpty) return;

    auto add = [&](const QString &lbl, QWidget *editor) {
        auto *l = new QLabel(lbl);
        l->setObjectName("fieldLabel");
        m_form->addRow(l, editor);
        editor->setVisible(true);
    };
    auto addHeader = [&](const QString &text) {
        if (m_view != ViewCategorized) return;
        auto *h = new QLabel(text);
        h->setObjectName("sectionHdr");
        m_form->addRow(h);
    };

    if (m_view == ViewCategorized) {
        add("Name", m_nameEdit);
        addHeader("LAYOUT");
        add("X", m_xSpin);
        add("Y", m_ySpin);
        add("Width",  m_wSpin);
        add("Height", m_hSpin);
        addHeader("COMMON");
        add(m_mode == ModeForm ? "Title" : "Text", m_textEdit);
        addHeader("APPEARANCE");
        add("Foreground", m_fgBtn);
        add("Background", m_bgBtn);
        if (m_mode == ModeWidget) {
            addHeader("BEHAVIOR");
            add("Visible", m_visibleRow);
            add("Anchor",  m_anchorRow);
        }
    } else {
        // Alphabetical — flat sorted list.
        QVector<QPair<QString, QWidget*>> rows;
        rows << QPair<QString,QWidget*>("Background", m_bgBtn);
        rows << QPair<QString,QWidget*>("Foreground", m_fgBtn);
        rows << QPair<QString,QWidget*>("Height",     m_hSpin);
        rows << QPair<QString,QWidget*>("Name",       m_nameEdit);
        rows << QPair<QString,QWidget*>(m_mode == ModeForm ? "Title" : "Text",
                                        m_textEdit);
        rows << QPair<QString,QWidget*>("Width",      m_wSpin);
        rows << QPair<QString,QWidget*>("X",          m_xSpin);
        rows << QPair<QString,QWidget*>("Y",          m_ySpin);
        if (m_mode == ModeWidget) {
            rows << QPair<QString,QWidget*>("Anchor",  m_anchorRow);
            rows << QPair<QString,QWidget*>("Visible", m_visibleRow);
        }
        std::sort(rows.begin(), rows.end(),
                  [](const QPair<QString,QWidget*> &a,
                     const QPair<QString,QWidget*> &b){
                       return a.first.toLower() < b.first.toLower();
                  });
        for (const auto &r : rows) add(r.first, r.second);
    }
}

void PropertyPanel::onSelectionChanged(QWidget *w) {
    if (w) {
        setMode(ModeWidget);
        refreshFromSelection();
        rebuildEventsSection();
    } else if (m_canvas && m_canvas->isFormSelected()) {
        // form selected — handled by onFormSelected too, but be defensive
        onFormSelected();
    } else {
        setMode(ModeEmpty);
    }
}

void PropertyPanel::onFormSelected() {
    setMode(ModeForm);
    refreshFromSelection();
    rebuildEventsSection();
}

void PropertyPanel::refreshFromSelection() {
    if (!m_canvas) return;
    m_updating = true;

    if (m_mode == ModeForm) {
        QSize fs = m_canvas->currentFormSize();
        m_typeLabel->setText("Form");
        m_nameEdit->setText(m_canvas->currentFormPath().isEmpty()
                              ? QString()
                              : QFileInfo(m_canvas->currentFormPath()).completeBaseName());
        m_nameEdit->setEnabled(false);          // form id is the file name
        m_textEdit->setText(m_canvas->currentFormTitle());
        m_xSpin->setValue(0); m_ySpin->setValue(0);
        m_wSpin->setValue(fs.width()); m_hSpin->setValue(fs.height());
        m_fgColor = m_canvas->formForeground(); updateColorBtn(m_fgBtn, m_fgColor);
        m_bgColor = m_canvas->formBackground(); updateColorBtn(m_bgBtn, m_bgColor);
    } else if (m_mode == ModeWidget) {
        QWidget *w = m_canvas->selectedWidget();
        if (!w) { m_updating = false; return; }
        m_typeLabel->setText(m_canvas->selectedType());
        m_nameEdit->setEnabled(true);
        m_nameEdit->setText(m_canvas->selectedName());
        m_xSpin->setValue(w->x()); m_ySpin->setValue(w->y());
        m_wSpin->setValue(w->width()); m_hSpin->setValue(w->height());
        if (WidgetFactory::hasTextProperty(m_canvas->selectedType())) {
            m_textEdit->setEnabled(true);
            m_textEdit->setText(WidgetFactory::readProperty(w, "text").toString());
        } else {
            m_textEdit->setEnabled(false);
            m_textEdit->setText(QString());
        }
        m_fgColor = WidgetFactory::readProperty(w, "fgColor").value<QColor>();
        updateColorBtn(m_fgBtn, m_fgColor);
        m_bgColor = WidgetFactory::readProperty(w, "bgColor").value<QColor>();
        updateColorBtn(m_bgBtn, m_bgColor);
        QVariant visV = WidgetFactory::readProperty(w, "visible");
        m_visibleCheck->setChecked(visV.isValid() ? visV.toBool() : true);
        applyAnchorString(WidgetFactory::readProperty(w, "anchor").toString());
    }

    m_updating = false;
}

void PropertyPanel::rebuildEventsSection() {
    // Clear existing rows
    while (auto *item = m_eventsLayout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    QStringList events;
    if (m_mode == ModeForm)        events << "Load" << "Unload";
    else if (m_mode == ModeWidget) events << "Click" << "DoubleClick" << "RightClick";
    for (const QString &e : events)
        m_eventsLayout->addWidget(makeEventRow(e));
}

QWidget *PropertyPanel::makeEventRow(const QString &eventName) {
    auto *row = new QWidget(m_eventsBox);
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0); h->setSpacing(8);

    auto *lbl = new QLabel(eventName + ":", row);
    lbl->setStyleSheet("color:#8a95a3; font-size:11px;");
    lbl->setMinimumWidth(86);
    h->addWidget(lbl);

    QString target = (m_mode == ModeForm)
        ? (m_canvas ? QFileInfo(m_canvas->currentFormPath()).completeBaseName() : QString("Form"))
        : (m_canvas ? m_canvas->selectedName() : QString());
    if (target.isEmpty()) target = (m_mode == ModeForm) ? "Form" : "Ctrl";

    auto *btn = new QPushButton(QString("+  %1_%2").arg(target, eventName), row);
    btn->setObjectName("evtBtn");
    btn->setCursor(Qt::PointingHandCursor);
    h->addWidget(btn, 1);

    QString eventCopy = eventName;
    QString targetCopy = target;
    connect(btn, &QPushButton::clicked, this, [this, targetCopy, eventCopy]{
        emit eventHandlerRequested(targetCopy, eventCopy);
    });
    return row;
}

// ── Field-edit handlers ───────────────────────────────────────────────────

void PropertyPanel::onNameEdited() {
    if (m_updating || !m_canvas || m_mode != ModeWidget) return;
    m_canvas->setNameForSelected(m_nameEdit->text().trimmed());
    rebuildEventsSection();   // handler names depend on the widget name
}

void PropertyPanel::onTitleEdited() {
    if (m_updating || !m_canvas) return;
    m_canvas->setFormTitle(m_textEdit->text());
}

void PropertyPanel::onTextEdited() {
    if (m_updating || !m_canvas) return;
    m_canvas->setTextForSelected(m_textEdit->text());
}

void PropertyPanel::onGeometryEdited() {
    if (m_updating || !m_canvas) return;
    QRect g(m_xSpin->value(), m_ySpin->value(),
            m_wSpin->value(), m_hSpin->value());
    if (m_mode == ModeForm)
        m_canvas->setFormGeometryFromPanel(g);
    else if (m_mode == ModeWidget)
        m_canvas->setGeometryForSelected(g);
}

void PropertyPanel::onFgClicked() {
    if (!m_canvas) return;
    QColor start = m_fgColor.isValid() ? m_fgColor : QColor("#000000");
    QColor c = QColorDialog::getColor(start, this, "Foreground colour",
                                      QColorDialog::ShowAlphaChannel);
    if (!c.isValid()) return;
    m_fgColor = c;
    updateColorBtn(m_fgBtn, c);
    if (m_mode == ModeForm)        m_canvas->setFormForeground(c);
    else if (m_mode == ModeWidget) m_canvas->setForegroundForSelected(c);
}

void PropertyPanel::onBgClicked() {
    if (!m_canvas) return;
    QColor start = m_bgColor.isValid() ? m_bgColor : QColor("#ffffff");
    QColor c = QColorDialog::getColor(start, this, "Background colour",
                                      QColorDialog::ShowAlphaChannel);
    if (!c.isValid()) return;
    m_bgColor = c;
    updateColorBtn(m_bgBtn, c);
    if (m_mode == ModeForm)        m_canvas->setFormBackground(c);
    else if (m_mode == ModeWidget) m_canvas->setBackgroundForSelected(c);
}

void PropertyPanel::onVisibleToggled(bool v) {
    if (m_updating || !m_canvas || m_mode != ModeWidget) return;
    m_canvas->setVisibleForSelected(v);
}

QString PropertyPanel::anchorString() const {
    QStringList parts;
    if (m_anchorT->isChecked()) parts << "Top";
    if (m_anchorL->isChecked()) parts << "Left";
    if (m_anchorR->isChecked()) parts << "Right";
    if (m_anchorB->isChecked()) parts << "Bottom";
    return parts.join(",");
}

void PropertyPanel::applyAnchorString(const QString &s) {
    QStringList parts = s.split(',', QString::SkipEmptyParts);
    m_anchorT->setChecked(parts.contains("Top",    Qt::CaseInsensitive));
    m_anchorL->setChecked(parts.contains("Left",   Qt::CaseInsensitive));
    m_anchorR->setChecked(parts.contains("Right",  Qt::CaseInsensitive));
    m_anchorB->setChecked(parts.contains("Bottom", Qt::CaseInsensitive));
}

void PropertyPanel::onAnchorToggled() {
    if (m_updating || !m_canvas || m_mode != ModeWidget) return;
    m_canvas->setAnchorForSelected(anchorString());
}
