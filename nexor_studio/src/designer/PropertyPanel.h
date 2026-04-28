// =============================================================================
// PropertyPanel — right-side panel in Design mode.
//
//   Properties (always shown when something is selected):
//     Type (read-only)
//     Name
//     X, Y, W, H              (W/H only — for the form, sets size)
//     Text                    (widgets with a text property; "Title" for form)
//     Foreground colour       (color picker)
//     Background colour       (color picker)
//     Visible                 (widgets only — hidden for the form)
//     Anchor                  (widgets only — Top / Left / Right / Bottom)
//
//   Events:
//     widgets : Click, DoubleClick, RightClick
//     form    : Load, Unload
//
//   Each event row has a "+ <handler-name>" button that asks MainWindow to
//   create the stub in the form's code and switch to Edit mode.
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
class QFormLayout;
class QVBoxLayout;

class PropertyPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertyPanel(QWidget *parent = nullptr);

    void setCanvas(FormCanvas *canvas);

signals:
    // Emitted when the user clicks "+ <handler>" in the Events section.
    //   targetName : widget name OR form id
    //   eventName  : "Click", "Load", ...
    void eventHandlerRequested(const QString &targetName, const QString &eventName);

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

private:
    enum Mode { ModeEmpty, ModeWidget, ModeForm };
    enum View { ViewCategorized, ViewAlphabetical };

    QPushButton *makeColorBtn();
    QToolButton *makeAnchorBtn(const QString &letter);
    QWidget     *makeEventRow(const QString &eventName);
    void         updateColorBtn(QPushButton *btn, const QColor &c);
    void         setMode(Mode m);
    void         setView(View v);
    void         rebuildFormLayout();
    void         rebuildEventsSection();
    QString      anchorString() const;
    void         applyAnchorString(const QString &s);

    // Cached label lookups — guarantees one QLabel widget per text,
    // so switching the categorized/alphabetical view doesn't pile up
    // orphaned QLabels on the panel.
    class QLabel *labelFor(const QString &text);
    class QLabel *headerFor(const QString &text);

    FormCanvas *m_canvas { nullptr };
    bool        m_updating { false };
    Mode        m_mode     { ModeEmpty };
    View        m_view     { ViewCategorized };
    QToolButton *m_btnCategorized;
    QToolButton *m_btnAlphabetical;
    class QFormLayout *m_form { nullptr };
    QHash<QString, class QLabel*> m_cachedLabels;
    QHash<QString, class QLabel*> m_cachedHeaders;

    // Property fields
    QLabel     *m_typeLabel;
    QLineEdit  *m_nameEdit;
    QLabel     *m_textLabel;       // label text changes between "Text" / "Title"
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

    QWidget     *m_visibleRow;     // hide for form mode
    QWidget     *m_anchorRow;      // hide for form mode

    // Events section
    QWidget     *m_eventsBox;
    QVBoxLayout *m_eventsLayout;

    QLabel      *m_emptyLabel;
};

#endif // NEXOR_STUDIO_PROPERTYPANEL_H
