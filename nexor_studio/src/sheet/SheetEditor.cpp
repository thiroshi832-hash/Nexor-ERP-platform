#include "SheetEditor.h"
#include "project/Sheet.h"

#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>

namespace {
const QStringList kTypes = {
    "String", "Long", "Integer", "Double", "Decimal",
    "Boolean", "Date", "Variant"
};
} // namespace

SheetEditor::SheetEditor(QWidget *parent) : QWidget(parent) {
    setupUi();
}

void SheetEditor::setupUi() {
    setStyleSheet(R"(
        SheetEditor { background:#13151b; }
        QLabel#sheetTitle {
            background:#0d0e12; color:#fbbf24;
            padding:10px 14px; border-bottom:1px solid #1e2030;
            font-family:"Segoe UI"; font-size:14px; font-weight:600;
            letter-spacing:1px;
        }
        QTableWidget#fieldsTable {
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
        QLineEdit, QComboBox {
            background:#262932; color:#dce1e7;
            border:1px solid #353945; border-radius:3px;
            padding:2px 6px; font-size:12px;
        }
        QCheckBox { background:transparent; color:#dce1e7; }
    )");

    auto *col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    m_titleLabel = new QLabel("SHEET", this);
    m_titleLabel->setObjectName("sheetTitle");
    col->addWidget(m_titleLabel);

    auto *body = new QWidget(this);
    auto *bodyCol = new QVBoxLayout(body);
    bodyCol->setContentsMargins(14, 14, 14, 14);
    bodyCol->setSpacing(10);

    m_table = new QTableWidget(0, 5, body);
    m_table->setObjectName("fieldsTable");
    m_table->setHorizontalHeaderLabels(
        QStringList() << "Name" << "Type" << "Key" << "Required" << "Default");
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(28);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnWidth(0, 160);
    m_table->setColumnWidth(1, 120);
    m_table->setColumnWidth(2, 60);
    m_table->setColumnWidth(3, 80);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    bodyCol->addWidget(m_table, 1);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);
    m_addBtn    = new QPushButton("+ Add field");
    m_removeBtn = new QPushButton("× Remove");
    m_saveBtn   = new QPushButton("Save");
    m_saveBtn->setObjectName("saveBtn");
    btnRow->addWidget(m_addBtn);
    btnRow->addWidget(m_removeBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_saveBtn);
    bodyCol->addLayout(btnRow);

    col->addWidget(body, 1);

    connect(m_addBtn,    &QPushButton::clicked, this, &SheetEditor::onAddField);
    connect(m_removeBtn, &QPushButton::clicked, this, &SheetEditor::onRemoveField);
    connect(m_saveBtn,   &QPushButton::clicked, this, &SheetEditor::onSaveClicked);
}

bool SheetEditor::loadSheet(const QString &filePath) {
    auto sht = std::make_unique<Sheet>();
    sht->setFilePath(filePath);
    if (!sht->load()) return false;
    m_sheet = std::move(sht);
    m_path  = filePath;
    m_titleLabel->setText("SHEET — " + m_sheet->meta().title);
    rebuildTable();
    return true;
}

void SheetEditor::clearSheet() {
    m_sheet.reset();
    m_path.clear();
    m_table->setRowCount(0);
    m_titleLabel->setText("SHEET");
}

void SheetEditor::rebuildTable() {
    m_table->setRowCount(0);
    if (!m_sheet) return;
    for (const FieldSpec &fs : m_sheet->fields()) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        // Name
        auto *nameEdit = new QLineEdit(fs.name);
        m_table->setCellWidget(row, 0, nameEdit);
        // Type
        auto *typeCombo = new QComboBox;
        typeCombo->addItems(kTypes);
        int idx = kTypes.indexOf(fs.type);
        typeCombo->setCurrentIndex(idx >= 0 ? idx : 0);
        m_table->setCellWidget(row, 1, typeCombo);
        // Key
        auto *keyHolder = new QWidget;
        auto *keyL = new QHBoxLayout(keyHolder);
        keyL->setContentsMargins(0,0,0,0); keyL->setAlignment(Qt::AlignCenter);
        auto *keyChk = new QCheckBox;
        keyChk->setChecked(fs.isKey);
        keyL->addWidget(keyChk);
        m_table->setCellWidget(row, 2, keyHolder);
        // Required
        auto *reqHolder = new QWidget;
        auto *reqL = new QHBoxLayout(reqHolder);
        reqL->setContentsMargins(0,0,0,0); reqL->setAlignment(Qt::AlignCenter);
        auto *reqChk = new QCheckBox;
        reqChk->setChecked(fs.required);
        reqL->addWidget(reqChk);
        m_table->setCellWidget(row, 3, reqHolder);
        // Default
        auto *defEdit = new QLineEdit(fs.defaultText);
        m_table->setCellWidget(row, 4, defEdit);
    }
}

void SheetEditor::writeBack() {
    if (!m_sheet) return;
    QVector<FieldSpec> out;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        FieldSpec fs;
        if (auto *e = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 0)))
            fs.name = e->text().trimmed();
        if (auto *c = qobject_cast<QComboBox*>(m_table->cellWidget(row, 1)))
            fs.type = c->currentText();
        if (auto *h = m_table->cellWidget(row, 2))
            if (auto *chk = h->findChild<QCheckBox*>())
                fs.isKey = chk->isChecked();
        if (auto *h = m_table->cellWidget(row, 3))
            if (auto *chk = h->findChild<QCheckBox*>())
                fs.required = chk->isChecked();
        if (auto *e = qobject_cast<QLineEdit*>(m_table->cellWidget(row, 4)))
            fs.defaultText = e->text();
        if (!fs.name.isEmpty()) out.append(fs);
    }
    m_sheet->fields() = out;
}

bool SheetEditor::saveSheet() {
    if (!m_sheet) return false;
    writeBack();
    return m_sheet->save();
}

void SheetEditor::onAddField() {
    if (!m_sheet) return;
    writeBack();
    FieldSpec fs;
    fs.name = QString("field%1").arg(m_sheet->fields().size() + 1);
    fs.type = "String";
    m_sheet->fields().append(fs);
    rebuildTable();
    emit modified();
}

void SheetEditor::onRemoveField() {
    int row = m_table->currentRow();
    if (row < 0 || !m_sheet) return;
    writeBack();
    if (row < m_sheet->fields().size()) {
        m_sheet->fields().removeAt(row);
        rebuildTable();
        emit modified();
    }
}

void SheetEditor::onSaveClicked() {
    saveSheet();
    emit modified();
}
