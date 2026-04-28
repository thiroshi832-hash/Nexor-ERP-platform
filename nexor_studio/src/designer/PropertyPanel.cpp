#include "PropertyPanel.h"
#include "FormCanvas.h"
#include "WidgetFactory.h"

#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QColorDialog>
#include <QFileInfo>
#include <QCoreApplication>
#include <QEvent>
#include <algorithm>

PropertyPanel::PropertyPanel(QWidget *parent) : QWidget(parent) {
    setObjectName("propertyPanel");
    // VB6 properties window — white card with a beige left column.
    setStyleSheet(R"(
        #propertyPanel { background:#ffffff; border-left:1px solid #808080;
                         font-family:"MS Sans Serif","Segoe UI"; }

        QLabel#paneHeader {
            background:#0d0e12; color:#8a95a3;
            padding:8px 12px; border-bottom:1px solid #1e2030;
            font-size:11px; font-weight:600; letter-spacing:2px;
        }

        QTableWidget#propTable {
            background:#ffffff;
            gridline-color:#B0B0B0;
            border:none; outline:0;
            font-family:"MS Sans Serif","Segoe UI";
            font-size:11px;
        }
        QTableWidget#propTable::item { padding:0 4px; }
        QTableWidget#propTable::item:selected {
            background:#0A246A; color:#ffffff;
        }

        QLineEdit, QSpinBox {
            background:#ffffff; color:#000000;
            border:none; padding:0 4px; font-size:11px;
        }
        QLineEdit:focus, QSpinBox:focus { background:#FFFFD0; }

        QToolButton#anchorBtn {
            background:#ECE9D8; color:#000000;
            border:1px solid #808080; border-radius:0;
            min-width:20px; min-height:14px;
            font-weight:600; font-size:10px;
        }
        QToolButton#anchorBtn:checked {
            background:#0A246A; color:#ffffff; border-color:#000040;
        }

        QPushButton#evtBtn {
            background:#ffffff; color:#0000FF;
            border:none; padding:0 4px;
            font-size:11px; text-align:left;
        }
        QPushButton#evtBtn:hover { background:#FFFFD0; }

        QComboBox#objCombo {
            background:#ffffff; color:#000000;
            border:1px solid #808080; border-radius:0;
            padding:2px 18px 2px 6px; font-size:11px;
            min-height:18px;
        }
        QComboBox#objCombo:focus    { border-color:#0A246A; }
        QComboBox#objCombo::drop-down { border:none; width:18px; }
        QComboBox QAbstractItemView {
            background:#ffffff; color:#000000;
            border:1px solid #808080;
            selection-background-color:#0A246A;
            selection-color:#ffffff;
        }

        #descArea { background:#ECE9D8; border-top:1px solid #808080;
                    border-bottom:1px solid #808080; }
        QLabel#descTitle { color:#000000; font-size:11px; font-weight:600;
                           padding:3px 6px 0 6px; }
        QLabel#descBody  { color:#404040; font-size:11px;
                           padding:0 6px 4px 6px; }
    )");

    auto *col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    // ── Header bar ─────────────────────────────────────────────────────
    auto *header = new QLabel("PROPERTIES", this);
    header->setObjectName("paneHeader");
    col->addWidget(header);

    // ── View toggle ────────────────────────────────────────────────────
    auto *toggleRow = new QWidget(this);
    toggleRow->setStyleSheet("background:#ECE9D8; border-bottom:1px solid #808080;");
    auto *toggleH = new QHBoxLayout(toggleRow);
    toggleH->setContentsMargins(8, 4, 8, 4); toggleH->setSpacing(4);
    auto makeViewBtn = [&](const QString &t) {
        auto *b = new QToolButton(toggleRow);
        b->setText(t); b->setCheckable(true);
        b->setStyleSheet(
            "QToolButton { background:transparent; color:#000000;"
            " border:none; padding:3px 10px; font-size:11px; }"
            "QToolButton:hover { color:#0A246A; }"
            "QToolButton:checked { color:#0A246A; font-weight:600;"
            " border-bottom:2px solid #0A246A; }");
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

    // ── Object combo ────────────────────────────────────────────────────
    m_objectCombo = new QComboBox(this);
    m_objectCombo->setObjectName("objCombo");
    {
        auto *holder = new QWidget(this);
        holder->setStyleSheet("background:#ECE9D8;");
        auto *h = new QHBoxLayout(holder);
        h->setContentsMargins(4, 4, 4, 4); h->setSpacing(0);
        h->addWidget(m_objectCombo);
        col->addWidget(holder);
    }
    connect(m_objectCombo, QOverload<int>::of(&QComboBox::activated),
            this, [this](int idx) {
        if (m_updating || !m_canvas || idx < 0) return;
        QString tag = m_objectCombo->itemData(idx, Qt::UserRole + 1).toString();
        if (tag == "form") {
            m_canvas->selectForm();
        } else if (tag == "widget") {
            QString name = m_objectCombo->itemData(idx, Qt::UserRole + 2).toString();
            m_canvas->selectByName(name);
        }
    });

    // ── Property table ─────────────────────────────────────────────────
    m_table = new QTableWidget(this);
    m_table->setObjectName("propTable");
    m_table->setColumnCount(2);
    m_table->horizontalHeader()->hide();
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(20);
    m_table->setShowGrid(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setFocusPolicy(Qt::StrongFocus);
    m_table->setAlternatingRowColors(false);
    m_table->setColumnWidth(0, 110);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_table->horizontalHeader()->setStretchLastSection(true);
    col->addWidget(m_table, 1);

    // Update description when the user selects a row
    connect(m_table, &QTableWidget::currentCellChanged, this,
            [this](int row, int, int, int) {
        if (row < 0) return;
        auto *it = m_table->item(row, 0);
        if (it) setDescription(it->text());
    });

    // ── Description pane ───────────────────────────────────────────────
    auto *descArea = new QWidget(this);
    descArea->setObjectName("descArea");
    auto *descCol = new QVBoxLayout(descArea);
    descCol->setContentsMargins(0, 0, 0, 0); descCol->setSpacing(0);
    m_descLabel = new QLabel("Properties", descArea);
    m_descLabel->setObjectName("descTitle");
    m_descBody  = new QLabel("Select a widget on the canvas, "
                              "or click the form to edit form properties.",
                              descArea);
    m_descBody->setObjectName("descBody");
    m_descBody->setWordWrap(true);
    m_descBody->setMinimumHeight(48);
    m_descBody->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    descCol->addWidget(m_descLabel);
    descCol->addWidget(m_descBody, 1);
    col->addWidget(descArea);

    // ── Editor widgets (members; reparented in/out of the table on rebuild)
    m_nameEdit = new QLineEdit(this);  m_nameEdit->setFrame(false);  m_nameEdit->hide();
    m_textLabel = nullptr;             // unused — kept in header for ABI
    m_textEdit = new QLineEdit(this);  m_textEdit->setFrame(false);  m_textEdit->hide();

    auto makeSpin = [&]() {
        auto *s = new QSpinBox(this);
        s->setRange(0, 4000);
        s->setFrame(false);
        s->setButtonSymbols(QAbstractSpinBox::NoButtons);
        s->setStyleSheet("QSpinBox{ background:white; border:none; padding:0 4px; }"
                         "QSpinBox:focus{ background:#FFFFD0; }");
        s->hide();
        return s;
    };
    m_xSpin = makeSpin(); m_ySpin = makeSpin();
    m_wSpin = makeSpin(); m_hSpin = makeSpin();

    m_fgBtn = makeColorBtn(); m_fgBtn->hide();
    m_bgBtn = makeColorBtn(); m_bgBtn->hide();

    m_visibleCheck = new QCheckBox(this);
    m_visibleCheck->setChecked(true);
    {
        m_visibleRow = new QWidget(this);
        m_visibleRow->setStyleSheet("background:white;");
        auto *h = new QHBoxLayout(m_visibleRow);
        h->setContentsMargins(4, 0, 4, 0); h->setSpacing(0);
        h->addWidget(m_visibleCheck);
        h->addStretch();
        m_visibleRow->hide();
    }

    m_anchorT = makeAnchorBtn("T");
    m_anchorL = makeAnchorBtn("L");
    m_anchorR = makeAnchorBtn("R");
    m_anchorB = makeAnchorBtn("B");
    {
        m_anchorRow = new QWidget(this);
        m_anchorRow->setStyleSheet("background:white;");
        auto *h = new QHBoxLayout(m_anchorRow);
        h->setContentsMargins(2, 0, 2, 0); h->setSpacing(2);
        h->addWidget(m_anchorT);
        h->addWidget(m_anchorL);
        h->addWidget(m_anchorR);
        h->addWidget(m_anchorB);
        h->addStretch();
        m_anchorRow->hide();
    }

    // Property descriptions (the iconic VB6 hint text).
    m_descriptions["Name"]       = "Returns the name used in code to identify an object.";
    m_descriptions["Text"]       = "Returns/sets the text displayed in this control.";
    m_descriptions["Title"]      = "Returns/sets the form's title bar text (Caption).";
    m_descriptions["X"]          = "Returns/sets the distance between the left edge of the control and the form.";
    m_descriptions["Y"]          = "Returns/sets the distance between the top edge of the control and the form.";
    m_descriptions["Width"]      = "Returns/sets the width of the object.";
    m_descriptions["Height"]     = "Returns/sets the height of the object.";
    m_descriptions["Foreground"] = "Returns/sets the foreground color used to display text and graphics.";
    m_descriptions["Background"] = "Returns/sets the background color used to display text and graphics.";
    m_descriptions["Visible"]    = "Returns/sets a value indicating whether an object is visible or hidden.";
    m_descriptions["Anchor"]     = "Returns/sets which edges of the parent the control sticks to when the form resizes.";
    m_descriptions["Click"]       = "Occurs when the user clicks the control with the mouse.";
    m_descriptions["DoubleClick"] = "Occurs when the user double-clicks the control with the mouse.";
    m_descriptions["RightClick"]  = "Occurs when the user right-clicks the control.";
    m_descriptions["Load"]        = "Fires once, when the form is first shown.";
    m_descriptions["Unload"]      = "Fires when the form is being closed.";

    // Wire field edits → canvas mutators
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
    b->setFixedHeight(18);
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
            "QPushButton { background:%1; color:%2; border:1px solid #808080;"
            " padding:0 6px; font-family:'MS Sans Serif'; font-size:11px;"
            " text-align:left; }")
            .arg(c.name(), fg));
        btn->setText(c.name().toUpper());
    } else {
        btn->setStyleSheet(
            "QPushButton { background:#ffffff; color:#808080;"
            " border:1px solid #C0C0C0; padding:0 6px;"
            " font-family:'MS Sans Serif'; font-size:11px; text-align:left; }");
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

    // Per-mode visibility (controlled here; rebuildLayout actually places
    // them in the table)
    bool isWidget = (m == ModeWidget);
    m_nameEdit->setEnabled(isWidget);    // form id is read-only (file name)
    m_visibleRow->setVisible(isWidget);
    m_anchorRow->setVisible(isWidget);

    rebuildLayout();
    populateObjectCombo();
}

void PropertyPanel::setView(View v) {
    m_view = v;
    rebuildLayout();
}

// ─── Table helpers ──────────────────────────────────────────────────────
int PropertyPanel::addPropertyRow(const QString &name, QWidget *editor) {
    int row = m_table->rowCount();
    m_table->insertRow(row);
    auto *item = new QTableWidgetItem(name);
    item->setBackground(QBrush(QColor(0xEC, 0xE9, 0xD8)));
    item->setForeground(QBrush(QColor(0x00, 0x00, 0x00)));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_table->setItem(row, 0, item);
    m_table->setCellWidget(row, 1, editor);
    editor->setVisible(true);
    return row;
}

int PropertyPanel::addSectionHeader(const QString &text) {
    int row = m_table->rowCount();
    m_table->insertRow(row);
    auto *item = new QTableWidgetItem(text);
    QFont f = item->font(); f.setBold(true);
    item->setFont(f);
    item->setBackground(QBrush(QColor(0xDC, 0xDC, 0xDC)));
    item->setFlags(Qt::ItemIsEnabled);
    m_table->setItem(row, 0, item);
    m_table->setSpan(row, 0, 1, 2);
    return row;
}

void PropertyPanel::detachAllEditorsFromTable() {
    // CRITICAL: QTableWidget::removeCellWidget() internally schedules a
    // deleteLater() on the widget that was in the cell.  If we leave that
    // event in the queue, the widget gets destroyed on the next event-loop
    // tick — long after we've re-attached it elsewhere — and the next access
    // crashes.  We cancel the pending DeferredDelete with removePostedEvents
    // so our persistent editors survive the layout rebuild.
    QList<QWidget*> editors {
        m_nameEdit, m_textEdit, m_xSpin, m_ySpin, m_wSpin, m_hSpin,
        m_fgBtn, m_bgBtn, m_visibleRow, m_anchorRow
    };
    for (QWidget *w : editors) {
        if (!w) continue;
        for (int r = 0; r < m_table->rowCount(); ++r) {
            if (m_table->cellWidget(r, 1) == w) {
                m_table->removeCellWidget(r, 1);
                QCoreApplication::removePostedEvents(w, QEvent::DeferredDelete);
                w->setParent(this);   // adopt back, keep alive
                w->hide();
                break;
            }
        }
    }
}

void PropertyPanel::rebuildLayout() {
    if (!m_table) return;
    detachAllEditorsFromTable();
    m_table->setRowCount(0);
    if (m_mode == ModeEmpty) return;

    auto add = [&](const QString &name, QWidget *editor) {
        addPropertyRow(name, editor);
    };
    auto hdr = [&](const QString &text) { addSectionHeader(text); };

    if (m_view == ViewCategorized) {
        add("Name", m_nameEdit);
        hdr("Layout");
        add("X",      m_xSpin);
        add("Y",      m_ySpin);
        add("Width",  m_wSpin);
        add("Height", m_hSpin);
        hdr("Common");
        add(m_mode == ModeForm ? "Title" : "Text", m_textEdit);
        hdr("Appearance");
        add("Foreground", m_fgBtn);
        add("Background", m_bgBtn);
        if (m_mode == ModeWidget) {
            hdr("Behavior");
            add("Visible", m_visibleRow);
            add("Anchor",  m_anchorRow);
        }
    } else {
        QVector<QPair<QString, QWidget*>> rows;
        rows << QPair<QString,QWidget*>("Background", m_bgBtn)
             << QPair<QString,QWidget*>("Foreground", m_fgBtn)
             << QPair<QString,QWidget*>("Height",     m_hSpin)
             << QPair<QString,QWidget*>("Name",       m_nameEdit)
             << QPair<QString,QWidget*>(m_mode == ModeForm ? "Title" : "Text",
                                        m_textEdit)
             << QPair<QString,QWidget*>("Width",      m_wSpin)
             << QPair<QString,QWidget*>("X",          m_xSpin)
             << QPair<QString,QWidget*>("Y",          m_ySpin);
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

    rebuildEventsInTable();
}

void PropertyPanel::rebuildEventsInTable() {
    addSectionHeader("Events");
    QStringList events;
    if (m_mode == ModeForm)        events << "Load" << "Unload";
    else if (m_mode == ModeWidget) events << "Click" << "DoubleClick" << "RightClick";

    QString target = (m_mode == ModeForm)
        ? (m_canvas ? QFileInfo(m_canvas->currentFormPath()).completeBaseName() : QString("Form"))
        : (m_canvas ? m_canvas->selectedName() : QString());
    if (target.isEmpty()) target = (m_mode == ModeForm) ? "Form" : "Ctrl";

    for (const QString &e : events) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        auto *nameItem = new QTableWidgetItem(e);
        nameItem->setBackground(QBrush(QColor(0xEC, 0xE9, 0xD8)));
        nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(row, 0, nameItem);

        // Per-build button — destroyed with the table row on next rebuild.
        auto *btn = new QPushButton(QString("+  %1_%2").arg(target, e));
        btn->setObjectName("evtBtn");
        btn->setCursor(Qt::PointingHandCursor);
        QString eCopy = e, tCopy = target;
        connect(btn, &QPushButton::clicked, this, [this, tCopy, eCopy]{
            emit eventHandlerRequested(tCopy, eCopy);
        });
        m_table->setCellWidget(row, 1, btn);
    }
}

// ─── Object combo + descriptions ────────────────────────────────────────
QString PropertyPanel::descriptionFor(const QString &fieldName) const {
    return m_descriptions.value(fieldName,
        "Edit this property to change the selected object.");
}

void PropertyPanel::setDescription(const QString &fieldName) {
    if (!m_descLabel || !m_descBody) return;
    m_descLabel->setText(fieldName);
    m_descBody->setText(descriptionFor(fieldName));
}

void PropertyPanel::populateObjectCombo() {
    if (!m_objectCombo) return;
    bool prev = m_updating;
    m_updating = true;
    m_objectCombo->clear();
    if (m_canvas && !m_canvas->currentFormPath().isEmpty()) {
        QString formId = QFileInfo(m_canvas->currentFormPath()).completeBaseName();
        m_objectCombo->addItem(formId + "  Form");
        int row = m_objectCombo->count() - 1;
        m_objectCombo->setItemData(row, "form", Qt::UserRole + 1);

        for (const auto &it : m_canvas->items()) {
            m_objectCombo->addItem(it.name + "  " + it.type);
            int r = m_objectCombo->count() - 1;
            m_objectCombo->setItemData(r, "widget", Qt::UserRole + 1);
            m_objectCombo->setItemData(r, it.name,  Qt::UserRole + 2);
        }

        int curIdx = 0;
        if (m_canvas->isFormSelected()) {
            curIdx = 0;
        } else if (m_canvas->selectedWidget()) {
            QString name = m_canvas->selectedName();
            for (int i = 0; i < m_objectCombo->count(); ++i)
                if (m_objectCombo->itemData(i, Qt::UserRole + 2).toString() == name) {
                    curIdx = i; break;
                }
        }
        m_objectCombo->setCurrentIndex(curIdx);
    }
    m_updating = prev;
}

// ─── Selection-change slots ─────────────────────────────────────────────
void PropertyPanel::onSelectionChanged(QWidget *w) {
    if (w) {
        setMode(ModeWidget);
        refreshFromSelection();
    } else if (m_canvas && m_canvas->isFormSelected()) {
        onFormSelected();
    } else {
        setMode(ModeEmpty);
    }
}

void PropertyPanel::onFormSelected() {
    setMode(ModeForm);
    refreshFromSelection();
}

void PropertyPanel::refreshFromSelection() {
    if (!m_canvas) return;
    populateObjectCombo();
    m_updating = true;

    if (m_mode == ModeForm) {
        QSize fs = m_canvas->currentFormSize();
        m_nameEdit->setText(m_canvas->currentFormPath().isEmpty()
            ? QString()
            : QFileInfo(m_canvas->currentFormPath()).completeBaseName());
        m_textEdit->setEnabled(true);
        m_textEdit->setText(m_canvas->currentFormTitle());
        m_xSpin->setValue(0); m_ySpin->setValue(0);
        m_wSpin->setValue(fs.width()); m_hSpin->setValue(fs.height());
        m_fgColor = m_canvas->formForeground(); updateColorBtn(m_fgBtn, m_fgColor);
        m_bgColor = m_canvas->formBackground(); updateColorBtn(m_bgBtn, m_bgColor);
    } else if (m_mode == ModeWidget) {
        QWidget *w = m_canvas->selectedWidget();
        if (!w) { m_updating = false; return; }
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

// ─── Field-edit handlers ────────────────────────────────────────────────
void PropertyPanel::onNameEdited() {
    setDescription("Name");
    if (m_updating || !m_canvas || m_mode != ModeWidget) return;
    m_canvas->setNameForSelected(m_nameEdit->text().trimmed());
}

void PropertyPanel::onTitleEdited() {
    setDescription("Title");
    if (m_updating || !m_canvas) return;
    m_canvas->setFormTitle(m_textEdit->text());
}

void PropertyPanel::onTextEdited() {
    setDescription(m_mode == ModeForm ? "Title" : "Text");
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
