#include "ProcessEditor.h"
#include "project/Process.h"
#include "editor/CodeEditor.h"

#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QComboBox>
#include <QLineEdit>

namespace {
const QStringList kTypes = { "Server", "Final" };
} // namespace

ProcessEditor::ProcessEditor(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void ProcessEditor::setupUi() {
    setStyleSheet(R"(
        ProcessEditor { background:#13151b; }
        QLabel#processTitle {
            background:#0d0e12; color:#c084fc;
            padding:10px 14px; border-bottom:1px solid #1e2030;
            font-family:"Segoe UI"; font-size:14px; font-weight:600;
            letter-spacing:1px;
        }
        QLabel#codeHeader {
            background:#0d0e12; color:#8a95a3;
            padding:6px 12px; border-top:1px solid #1e2030;
            border-bottom:1px solid #1e2030;
            font-size:11px; font-weight:600; letter-spacing:2px;
        }
        QTableWidget#stepsTable {
            background:#1b1d23; color:#dce1e7;
            gridline-color:#2a3655;
            border:none; outline:0;
            font-family:"Segoe UI"; font-size:12px;
        }
        QHeaderView::section {
            background:#262932; color:#8a95a3;
            padding:5px 8px; border:none; border-right:1px solid #1e2030;
            font-weight:600; font-size:11px; letter-spacing:1px;
        }
        QTableWidget::item:selected { background:#1e3a5f; color:#ffffff; }
        QPushButton {
            background:#262932; color:#dce1e7;
            border:1px solid #353945; border-radius:4px;
            padding:5px 14px; font-size:12px;
        }
        QPushButton:hover         { background:#2d3140; border-color:#5b8cff; }
        QPushButton#saveBtn       { background:#1e3a5f; }
        QPushButton#saveBtn:hover { background:#26477a; }
        QPushButton#runBtn        { background:#3a1e5f; }
        QPushButton#runBtn:hover  { background:#4d2779; }
        QLineEdit, QComboBox {
            background:#262932; color:#dce1e7;
            border:1px solid #353945; border-radius:3px;
            padding:2px 6px; font-size:12px;
        }
    )");

    auto *col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    // Header row with title + Run button
    auto *headerRow = new QWidget(this);
    headerRow->setStyleSheet("background:#0d0e12; border-bottom:1px solid #1e2030;");
    auto *hRow = new QHBoxLayout(headerRow);
    hRow->setContentsMargins(0, 0, 12, 0); hRow->setSpacing(8);
    m_titleLabel = new QLabel("PROCESS", headerRow);
    m_titleLabel->setObjectName("processTitle");
    m_titleLabel->setStyleSheet("border-bottom:none;");
    hRow->addWidget(m_titleLabel, 1);
    m_runBtn = new QPushButton("▶ Run Process", headerRow);
    m_runBtn->setObjectName("runBtn");
    hRow->addWidget(m_runBtn);
    col->addWidget(headerRow);

    auto *split = new QSplitter(Qt::Vertical, this);
    split->setHandleWidth(1);
    split->setChildrenCollapsible(false);
    split->setStyleSheet("QSplitter::handle{ background:#1e2030; }");

    // Top: steps table + buttons
    auto *top = new QWidget;
    auto *topCol = new QVBoxLayout(top);
    topCol->setContentsMargins(14, 14, 14, 14); topCol->setSpacing(10);

    m_table = new QTableWidget(0, 3, top);
    m_table->setObjectName("stepsTable");
    m_table->setHorizontalHeaderLabels(QStringList() << "ID" << "Type" << "Next");
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(28);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnWidth(0, 200);
    m_table->setColumnWidth(1, 120);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    topCol->addWidget(m_table, 1);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);
    m_addBtn    = new QPushButton("+ Add step");
    m_removeBtn = new QPushButton("× Remove");
    m_saveBtn   = new QPushButton("Save");
    m_saveBtn->setObjectName("saveBtn");
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_removeBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_saveBtn);
    topCol->addLayout(btnRow);

    split->addWidget(top);

    // Bottom: per-step code editor
    auto *bot = new QWidget;
    auto *botCol = new QVBoxLayout(bot);
    botCol->setContentsMargins(0, 0, 0, 0); botCol->setSpacing(0);
    m_codeHeader = new QLabel("STEP CODE", bot);
    m_codeHeader->setObjectName("codeHeader");
    botCol->addWidget(m_codeHeader);
    m_codeEditor = new CodeEditor(bot);
    botCol->addWidget(m_codeEditor, 1);
    split->addWidget(bot);

    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 1);
    split->setSizes({ 300, 300 });
    col->addWidget(split, 1);

    connect(m_addBtn,    &QPushButton::clicked, this, &ProcessEditor::onAddStep);
    connect(m_removeBtn, &QPushButton::clicked, this, &ProcessEditor::onRemoveStep);
    connect(m_saveBtn,   &QPushButton::clicked, this, &ProcessEditor::onSaveClicked);
    connect(m_runBtn,    &QPushButton::clicked, this, &ProcessEditor::onRunClicked);
    connect(m_table, &QTableWidget::currentCellChanged, this,
            [this](int row, int, int, int){ onRowChanged(row); });
    connect(m_codeEditor, &CodeEditor::textChanged,
            this, &ProcessEditor::onCodeChanged);
}

bool ProcessEditor::loadProcess(const QString &filePath) {
    auto prc = std::make_unique<Process>();
    prc->setFilePath(filePath);
    if (!prc->load()) return false;
    m_process = std::move(prc);
    m_path  = filePath;
    m_titleLabel->setText("PROCESS — " + m_process->meta().title);
    m_activeRow = -1;
    rebuildTable();
    if (m_table->rowCount() > 0) m_table->selectRow(0);
    return true;
}

void ProcessEditor::clearProcess() {
    m_process.reset();
    m_path.clear();
    m_table->setRowCount(0);
    m_codeEditor->clear();
    m_titleLabel->setText("PROCESS");
    m_codeHeader->setText("STEP CODE");
    m_activeRow = -1;
}

void ProcessEditor::rebuildTable() {
    m_table->blockSignals(true);
    m_table->setRowCount(0);
    if (!m_process) { m_table->blockSignals(false); return; }
    for (const StepSpec &st : m_process->steps()) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        // ID
        auto *idEdit = new QLineEdit(st.id);
        m_table->setCellWidget(row, 0, idEdit);
        // Type
        auto *typeCombo = new QComboBox;
        typeCombo->addItems(kTypes);
        int idx = kTypes.indexOf(st.type);
        typeCombo->setCurrentIndex(idx >= 0 ? idx : 0);
        m_table->setCellWidget(row, 1, typeCombo);
        // Next
        auto *nextEdit = new QLineEdit(st.nextId);
        m_table->setCellWidget(row, 2, nextEdit);
    }
    m_table->blockSignals(false);
}

void ProcessEditor::writeBack() {
    if (!m_process) return;
    QVector<StepSpec> &out = m_process->steps();
    int rows = m_table->rowCount();
    if (rows != out.size()) return;     // table rebuild in flight
    for (int row = 0; row < rows; ++row) {
        StepSpec &st = out[row];
        if (auto *e = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 0)))
            st.id = e->text().trimmed();
        if (auto *c = qobject_cast<QComboBox*>(m_table->cellWidget(row, 1)))
            st.type = c->currentText();
        if (auto *e = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 2)))
            st.nextId = e->text().trimmed();
    }
}

bool ProcessEditor::saveProcess() {
    if (!m_process) return false;
    // Push current code editor text into the active step before serialising.
    if (m_activeRow >= 0 && m_activeRow < m_process->steps().size())
        m_process->steps()[m_activeRow].code = m_codeEditor->toPlainText();
    writeBack();
    return m_process->save();
}

void ProcessEditor::onAddStep() {
    if (!m_process) return;
    if (m_activeRow >= 0 && m_activeRow < m_process->steps().size())
        m_process->steps()[m_activeRow].code = m_codeEditor->toPlainText();
    writeBack();
    StepSpec st;
    st.id = QString("step%1").arg(m_process->steps().size() + 1);
    st.type = "Server";
    st.code = "    ' New step\n";
    m_process->steps().append(st);
    rebuildTable();
    m_table->selectRow(m_table->rowCount() - 1);
    emit modified();
}

void ProcessEditor::onRemoveStep() {
    int row = m_table->currentRow();
    if (row < 0 || !m_process) return;
    if (m_activeRow >= 0 && m_activeRow < m_process->steps().size()
        && m_activeRow != row)
        m_process->steps()[m_activeRow].code = m_codeEditor->toPlainText();
    writeBack();
    if (row < m_process->steps().size()) {
        m_process->steps().removeAt(row);
        m_activeRow = -1;
        rebuildTable();
        if (m_table->rowCount() > 0)
            m_table->selectRow(qMin(row, m_table->rowCount() - 1));
        else
            m_codeEditor->clear();
        emit modified();
    }
}

void ProcessEditor::onSaveClicked() {
    saveProcess();
    emit modified();
}

void ProcessEditor::onRunClicked() {
    saveProcess();
    if (!m_path.isEmpty()) emit runRequested(m_path);
}

void ProcessEditor::onRowChanged(int row) {
    if (!m_process) return;
    // Persist the previously-shown step's code before swapping.
    if (m_activeRow >= 0 && m_activeRow < m_process->steps().size())
        m_process->steps()[m_activeRow].code = m_codeEditor->toPlainText();

    m_activeRow = row;
    if (row < 0 || row >= m_process->steps().size()) {
        m_codeEditor->clear();
        m_codeHeader->setText("STEP CODE");
        return;
    }
    const StepSpec &st = m_process->steps().at(row);
    m_codeHeader->setText(QString("STEP CODE — %1").arg(st.id));
    m_codeEditor->blockSignals(true);
    m_codeEditor->setPlainText(st.code);
    m_codeEditor->blockSignals(false);
}

void ProcessEditor::onCodeChanged() {
    if (!m_process) return;
    if (m_activeRow >= 0 && m_activeRow < m_process->steps().size())
        m_process->steps()[m_activeRow].code = m_codeEditor->toPlainText();
}
