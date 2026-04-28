#include "EditorView.h"
#include "CodeEditor.h"

#include "designer/FormCanvas.h"
#include "designer/WidgetFactory.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QFileInfo>

namespace {
const char *kGeneral      = "(General)";
const char *kDeclarations = "(declarations)";
} // namespace

EditorView::EditorView(QWidget *parent)
    : QWidget(parent)
    , m_objectCombo(nullptr)
    , m_procCombo(nullptr)
    , m_editor(nullptr) {

    setStyleSheet(R"(
        QComboBox {
            background:#262932; color:#dce1e7;
            border:1px solid #353945; border-radius:3px;
            padding:3px 22px 3px 8px; font-size:12px;
            min-height:18px;
        }
        QComboBox:hover, QComboBox:focus { border-color:#5b8cff; }
        QComboBox::drop-down { border:none; width:18px; }
        QComboBox QAbstractItemView {
            background:#1b1d23; color:#dce1e7;
            border:1px solid #2a3655; selection-background-color:#1e3a5f;
        }
    )");

    auto *col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    // ── Header bar with two dropdowns ────────────────────────────────
    auto *bar = new QWidget(this);
    bar->setStyleSheet("background:#181a22; border-bottom:1px solid #1e2030;");
    auto *barRow = new QHBoxLayout(bar);
    barRow->setContentsMargins(8, 6, 8, 6);
    barRow->setSpacing(6);

    m_objectCombo = new QComboBox(bar);
    m_objectCombo->setMinimumWidth(180);
    m_objectCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    barRow->addWidget(m_objectCombo, 1);

    auto *sep = new QFrame(bar);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("color:#2a3655;");
    barRow->addWidget(sep);

    m_procCombo = new QComboBox(bar);
    m_procCombo->setMinimumWidth(180);
    m_procCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    barRow->addWidget(m_procCombo, 1);

    col->addWidget(bar);

    // ── Editor below ─────────────────────────────────────────────────
    m_editor = new CodeEditor(this);
    col->addWidget(m_editor, 1);

    connect(m_objectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditorView::onObjectChanged);
    connect(m_procCombo,   QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditorView::onProcedureChanged);

    refresh();
}

void EditorView::setFormCanvas(FormCanvas *canvas) {
    if (m_canvas) disconnect(m_canvas, nullptr, this, nullptr);
    m_canvas = canvas;
    if (m_canvas) {
        // Widget add/remove/rename — refresh the Object list.
        connect(m_canvas, &FormCanvas::modified, this, &EditorView::refresh);
    }
    refresh();
}

void EditorView::refresh() {
    m_updating = true;

    m_objectCombo->clear();
    m_procCombo->clear();

    // Always have (General).
    m_objectCombo->addItem(kGeneral);
    m_objectCombo->setItemData(0, "general", Qt::UserRole + 1);   // kind tag

    // If editor has a form open, list Form + widgets.
    if (m_editor && m_editor->currentKind() == CodeEditor::KindForm
     && m_canvas && !m_canvas->currentFormPath().isEmpty()) {
        QString formId =
            QFileInfo(m_canvas->currentFormPath()).completeBaseName();
        m_objectCombo->addItem(formId);
        int formRow = m_objectCombo->count() - 1;
        m_objectCombo->setItemData(formRow, "form", Qt::UserRole + 1);
        m_objectCombo->setItemData(formRow, formId, Qt::UserRole + 2);

        for (const auto &it : m_canvas->items()) {
            m_objectCombo->addItem(it.name);
            int row = m_objectCombo->count() - 1;
            m_objectCombo->setItemData(row, "widget",  Qt::UserRole + 1);
            m_objectCombo->setItemData(row, it.name,   Qt::UserRole + 2);
            m_objectCombo->setItemData(row, it.type,   Qt::UserRole + 3);
        }
    }

    populateProcedures();
    m_updating = false;
}

void EditorView::populateProcedures() {
    bool wasUpdating = m_updating;
    m_updating = true;
    m_procCombo->clear();

    int idx = m_objectCombo->currentIndex();
    QString kind = (idx < 0)
        ? QString("general")
        : m_objectCombo->itemData(idx, Qt::UserRole + 1).toString();

    QStringList procs;
    if (kind == "general") {
        procs << kDeclarations;
    } else if (kind == "form") {
        procs << "Load" << "Unload";
    } else {  // widget
        QString type = m_objectCombo->itemData(idx, Qt::UserRole + 3).toString();
        procs = WidgetFactory::eventsFor(type);
    }
    m_procCombo->addItems(procs);
    m_updating = wasUpdating;
}

void EditorView::onObjectChanged(int) {
    populateProcedures();
}

void EditorView::onProcedureChanged(int) {
    if (m_updating) return;
    int oi = m_objectCombo->currentIndex();
    if (oi < 0) return;
    QString kind = m_objectCombo->itemData(oi, Qt::UserRole + 1).toString();
    if (kind == "general") return;          // (declarations) — nothing to do

    QString target = m_objectCombo->itemData(oi, Qt::UserRole + 2).toString();
    QString event  = m_procCombo->currentText();
    if (target.isEmpty() || event.isEmpty()) return;

    emit eventHandlerRequested(target, event);
}
