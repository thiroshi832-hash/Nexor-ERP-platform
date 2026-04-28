// =============================================================================
// PropertyPanel — VB6-style properties window built on a QTableWidget.
//
//   ┌────────────────────────────────┐
//   │ PROPERTIES                     │  ← header
//   ├────────────────────────────────┤
//   │ [Categorized] [Alphabetical]   │  ← view tabs
//   ├────────────────────────────────┤
//   │ ▾ btn1  Button                 │  ← object combo
//   ├──────────────┬─────────────────┤
//   │ Name         │ btn1            │
//   │ ─── Layout ─────────────────── │
//   │ X            │ 24              │
//   │ ...          │ ...             │
//   ├──────────────┴─────────────────┤
//   │ Width                          │  ← description (selected row)
//   │ Returns/sets the width of...   │
//   └────────────────────────────────┘
// =============================================================================
#ifndef NEXOR_STUDIO_PROPERTYPANEL_H
#define NEXOR_STUDIO_PROPERTYPANEL_H

#include <QWidget>
#include <QColor>
#include <QHash>
#include <QString>

class FormCanvas;
class QLabel;
class QLineEdit;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QToolButton;
class QTableWidget;
class QTableWidgetItem;
class QComboBox;
class QVBoxLayout;

class PropertyPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertyPanel(QWidget *parent = nullptr);

    void setCanvas(FormCanvas *canvas);

signals:
    void eventHandlerRequested(const QString &targetName,
                               const QString &eventName);

public slots:
    void onSelectionChanged(QWidget *w);
    void onFormSelected();
    void refreshFromSelection();

private slots:
    void onNameEdited();
    void onTitleEdited();
    void onTextEdited();
    void onGeometryEdited();
    void onFgClicked();
    void onBgClicked();
    void onVisibleToggled(bool v);
    void onAnchorToggled();
    void onDataSourceEdited();
    void onBindingEdited();

private:
    enum Mode { ModeEmpty, ModeWidget, ModeForm };
    enum View { ViewCategorized, ViewAlphabetical };

    QPushButton *makeColorBtn();
    QToolButton *makeAnchorBtn(const QString &letter);
    void         updateColorBtn(QPushButton *btn, const QColor &c);

    void setMode(Mode m);
    void setView(View v);
    void rebuildLayout();
    QString anchorString() const;
    void applyAnchorString(const QString &s);

    // QTableWidget helpers
    int  addPropertyRow(const QString &name, QWidget *editor);
    int  addSectionHeader(const QString &text);
    void detachAllEditorsFromTable();
    void rebuildEventsInTable();

    void populateObjectCombo();
    QString descriptionFor(const QString &fieldName) const;
    void    setDescription(const QString &fieldName);

    FormCanvas *m_canvas { nullptr };
    bool        m_updating { false };
    Mode        m_mode     { ModeEmpty };
    View        m_view     { ViewCategorized };

    QToolButton  *m_btnCategorized;
    QToolButton  *m_btnAlphabetical;
    QComboBox    *m_objectCombo  { nullptr };
    QTableWidget *m_table        { nullptr };
    QLabel       *m_descLabel    { nullptr };
    QLabel       *m_descBody     { nullptr };

    // Editor widgets (persist across rebuilds; reparented in/out of table)
    QLineEdit  *m_nameEdit;
    QLabel     *m_textLabel;        // unused now (label comes from item)
    QLineEdit  *m_textEdit;
    QSpinBox   *m_xSpin;
    QSpinBox   *m_ySpin;
    QSpinBox   *m_wSpin;
    QSpinBox   *m_hSpin;
    QPushButton *m_fgBtn;
    QPushButton *m_bgBtn;
    QColor      m_fgColor;
    QColor      m_bgColor;
    QCheckBox  *m_visibleCheck;
    QToolButton *m_anchorT;
    QToolButton *m_anchorL;
    QToolButton *m_anchorR;
    QToolButton *m_anchorB;
    QWidget     *m_visibleRow;       // wrapper around m_visibleCheck
    QWidget     *m_anchorRow;        // wrapper around the 4 anchor buttons
    QLineEdit   *m_dataSourceEdit;   // form-only: which Sheet this form edits
    QLineEdit   *m_bindingEdit;      // widget-only: which entity field

    QHash<QString, QString> m_descriptions;
};

#endif // NEXOR_STUDIO_PROPERTYPANEL_H
