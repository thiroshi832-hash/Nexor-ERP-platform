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

    explicit FormCanvas(QWidget *parent = nullptr);
    ~FormCanvas() override;

    bool   loadForm(const QString &filePath);
    bool   saveForm();
    void   clearForm();

    QString currentFormPath()  const { return m_path; }
    QString currentFormTitle() const { return m_title; }
    QSize   currentFormSize()  const { return QSize(m_formW, m_formH); }

    void    setFormTitle(const QString &t);
    void    setFormSize(const QSize &s);

    QWidget* selectedWidget() const { return m_selected; }
    QString  selectedName()   const;
    QString  selectedType()   const;

    // Mutators called by the PropertyPanel.
    void setNameForSelected(const QString &n);
    void setTextForSelected(const QString &t);
    void setGeometryForSelected(const QRect &g);

    // Public to allow the FormRunner to walk the design.
    struct Item { QString type; QString name; QWidget *widget; };
    const QVector<Item>& items() const { return m_items; }

    QSize sizeHint() const override;

    // Form-level event-driven code (Form_Load, btnX_Click, ...) — lives in
    // the .frm file alongside the widgets.  The CodeEditor edits this string
    // when in "Form mode" and pushes back via setCode() before save.
    QString code() const { return m_code; }
    void    setCode(const QString &c) { m_code = c; emit modified(); }

signals:
    void selectionChanged(QWidget *w);
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
    int     m_formW { 640 };
    int     m_formH { 480 };

    // ── Visual chrome constants ──────────────────────────────────────────
    static constexpr int kTitleBarH = 28;
    static constexpr int kPadding   = 30;

    // ── Children ─────────────────────────────────────────────────────────
    QWidget        *m_body;
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
