// =============================================================================
// FormCanvas — live form design surface.
//
// Real Qt widgets are placed on a "body" sub-widget that represents the
// form's client area.  The user can drag widgets in from the WidgetPalette,
// click to select, drag to move, drag the 8 selection handles to resize, and
// hit Delete to remove.  What you design is byte-for-byte what the form
// shows when run because both code paths use the same WidgetFactory::create().
//
// Layout:
//
//   FormCanvas (canvas-area, dark background)
//     └─ m_body (QWidget, w×h = formW × formH, light background)
//          └─ designed widgets as direct children, geometry in body coords
//     └─ 8 selection handles (small overlay widgets, raised above body)
//     └─ painted title bar above m_body
//
// =============================================================================
#ifndef NEXOR_STUDIO_FORMCANVAS_H
#define NEXOR_STUDIO_FORMCANVAS_H

#include <QWidget>
#include <QVector>
#include <QString>
#include <QPoint>
#include <QRect>

class FormCanvas : public QWidget {
    Q_OBJECT
public:
    class SelHandle;            // nested resize-handle widget, defined in .cpp
    class FormBody;             // nested form-area widget, defined in .cpp

    explicit FormCanvas(QWidget *parent = nullptr);
    ~FormCanvas() override;

    bool   loadForm(const QString &filePath);
    bool   saveForm();
    void   clearForm();

    QString currentFormPath()  const { return m_path; }
    QString currentFormTitle() const { return m_title; }
    QSize   currentFormSize()  const { return QSize(m_formW, m_formH); }

    // The Sheet/entity this form edits (empty = no binding).
    QString dataSource()       const { return m_dataSource; }
    void    setDataSource(const QString &s);

    void    setFormTitle(const QString &t);
    void    setFormSize(const QSize &s);

    QWidget* selectedWidget() const { return m_selected; }
    bool     isFormSelected() const { return m_formSelected; }
    QString  selectedName()   const;
    QString  selectedType()   const;
    void     selectForm();         // selects the form itself (for property panel)
    void     selectByName(const QString &name);   // pick widget by item name

    // Per-widget mutators called by the PropertyPanel.
    void setNameForSelected(const QString &n);
    void setTextForSelected(const QString &t);
    void setGeometryForSelected(const QRect &g);
    void setForegroundForSelected(const QColor &c);
    void setBackgroundForSelected(const QColor &c);
    void setVisibleForSelected(bool visible);
    void setAnchorForSelected(const QString &anchor);

    // Form-as-target mutators.
    QColor formForeground() const { return m_formFg; }
    QColor formBackground() const { return m_formBg; }
    void   setFormForeground(const QColor &c);
    void   setFormBackground(const QColor &c);
    void   setFormGeometryFromPanel(const QRect &g);   // for X/Y/W/H of form

    // Public to allow the FormRunner to walk the design.
    struct Item { QString type; QString name; QString binding; QWidget *widget; };
    const QVector<Item>& items() const { return m_items; }

    // Sets the binding (entity field name) for the currently-selected widget.
    void setBindingForSelected(const QString &fieldName);
    QString bindingForSelected() const;

    // Reorders m_items to match the supplied widget-name list.  Also calls
    // QWidget::setTabOrder so the actual focus chain reflects the new order.
    void reorderItems(const QStringList &names);

    QSize sizeHint() const override;

    int  gridSize() const     { return m_gridSize; }
    void setGridSize(int n)   { m_gridSize = qMax(1, n); update(); }
    bool snapEnabled() const  { return m_snapEnabled; }
    void setSnapEnabled(bool on) { m_snapEnabled = on; }

    // Form-level event-driven code (Form_Load, btnX_Click, ...) — lives in
    // the .frm file alongside the widgets.  The CodeEditor edits this string
    // when in "Form mode" and pushes back via setCode() before save.
    QString code() const { return m_code; }
    void    setCode(const QString &c) { m_code = c; emit modified(); }

signals:
    void selectionChanged(QWidget *w);   // null when nothing OR form selected
    void formSelected();                  // emitted when the form is picked
    void widgetDoubleClicked(const QString &name, const QString &type);
    void formDoubleClicked();             // double-click empty form area
    void modified();

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dropEvent(QDropEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void    layoutBody();
    void    layoutHandles();
    void    selectWidget(QWidget *w);
    void    deleteSelected();
    QString uniqueName(const QString &prefix);
    void    cleanupAllWidgets();
    int     itemIndexFor(QWidget *w) const;
    QPoint  bodyOrigin() const;
    QRect   formChromeRect() const;

    // Creates a designed widget on m_body at the given body-local position
    // (auto-clamped, auto-named, auto-selected).  Used by both the canvas-
    // level dropEvent and the m_body event filter.
    void createWidgetAt(const QString &type, const QPoint &bodyCenterPos);

    // ── Form spec ────────────────────────────────────────────────────────
    QString m_path;
    QString m_id;
    QString m_title;
    QString m_code;
    QString m_dataSource;     // Sheet id (empty = no entity binding)
    QColor  m_formFg;
    QColor  m_formBg;
    int     m_formW { 640 };
    int     m_formH { 480 };
    bool    m_formSelected { false };
    int     m_gridSize    { 8 };
    bool    m_snapEnabled { true };

    static int snapTo(int v, int g) { return ((v + g/2) / g) * g; }
    QRect snapRect(const QRect &r) const;
    QRect snapMove(const QRect &r) const;

    // ── Visual chrome constants ──────────────────────────────────────────
    // No title bar in design view — VB6 / WinForms style: the canvas just
    // shows the form's client area surrounded by a raised bezel.
    static constexpr int kTitleBarH = 0;
    static constexpr int kPadding   = 30;

    // ── Children ─────────────────────────────────────────────────────────
    FormBody       *m_body;
    QVector<Item>   m_items;
    QWidget        *m_selected { nullptr };
    QVector<SelHandle*> m_handles;     // size = 8

    // ── Drag/move/resize state ───────────────────────────────────────────
    enum Op { OpNone, OpMove, OpResize };
    Op     m_op         { OpNone };
    int    m_resizeDir  { -1 };        // 0..7 corresponds to SelHandle::Dir
    QPoint m_pressGlobal;
    QRect  m_pressGeom;
};

#endif // NEXOR_STUDIO_FORMCANVAS_H
