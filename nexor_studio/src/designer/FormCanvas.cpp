#include "FormCanvas.h"
#include "WidgetFactory.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QSet>
#include <QColor>
#include <QHash>

// =============================================================================
// FormCanvas::SelHandle — small overlay widget at one of 8 positions around
// the currently-selected designed widget.  Black 7×7 filled squares — the
// classic VB6 selection-handle look.
// =============================================================================
class FormCanvas::SelHandle : public QWidget {
public:
    enum Dir { TL = 0, T, TR, R, BR, B, BL, L };

    SelHandle(Dir d, QWidget *parent) : QWidget(parent), m_dir(d) {
        setFixedSize(7, 7);
        setStyleSheet("background:#000000;");
        switch (d) {
            case TL: case BR: setCursor(Qt::SizeFDiagCursor); break;
            case TR: case BL: setCursor(Qt::SizeBDiagCursor); break;
            case T : case B : setCursor(Qt::SizeVerCursor);   break;
            case L : case R : setCursor(Qt::SizeHorCursor);   break;
        }
    }
    Dir direction() const { return m_dir; }

private:
    Dir m_dir;
};

// =============================================================================
// FormCanvas::FormBody — the form's client area.  Paints the VB6 dot grid.
// =============================================================================
class FormCanvas::FormBody : public QWidget {
public:
    FormBody(QWidget *parent)
        : QWidget(parent), m_bg(0xC0, 0xC0, 0xC0) {}

    void setBg(const QColor &c) {
        m_bg = c.isValid() ? c : QColor(0xC0, 0xC0, 0xC0);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        // Form interior — VB6 default form colour (button face gray).
        p.fillRect(rect(), m_bg);
        // 1-px dark border so the form edge reads against the canvas.
        p.setPen(QColor(0x40, 0x40, 0x40));
        p.drawRect(rect().adjusted(0, 0, -1, -1));
        // Dot grid: black points at every 8-pixel intersection.
        p.setPen(QColor(0x00, 0x00, 0x00));
        const int g = 8;
        for (int y = g; y < height() - 1; y += g)
            for (int x = g; x < width() - 1; x += g)
                p.drawPoint(x, y);
    }

private:
    QColor m_bg;
};

// =============================================================================
// FormCanvas
// =============================================================================
FormCanvas::FormCanvas(QWidget *parent) : QWidget(parent) {
    setObjectName("formCanvas");
    // VB6 MDI workspace gray.
    setStyleSheet("QWidget#formCanvas { background:#808080; }");
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    setMouseTracking(true);

    // Body: the form's client area.  FormBody paints itself (dot grid).
    m_body = new FormBody(this);
    m_body->setObjectName("formBody");
    m_body->setMouseTracking(true);
    m_body->setAcceptDrops(true);          // accept palette drops on body
    m_body->installEventFilter(this);
    m_body->resize(m_formW, m_formH);
    m_body->hide();    // no form is loaded yet — body should not show

    // 8 selection handles.
    for (int d = 0; d < 8; ++d) {
        auto *h = new SelHandle(static_cast<SelHandle::Dir>(d), this);
        h->hide();
        h->installEventFilter(this);
        m_handles.append(h);
    }

    layoutBody();
}

FormCanvas::~FormCanvas() = default;

QSize FormCanvas::sizeHint() const {
    return QSize(m_formW + 2 * kPadding,
                 m_formH + kTitleBarH + 2 * kPadding);
}

QPoint FormCanvas::bodyOrigin() const {
    int x = qMax(kPadding, (width()  - m_formW) / 2);
    int y = qMax(kPadding + kTitleBarH,
                 (height() - m_formH - kTitleBarH) / 2 + kTitleBarH);
    return {x, y};
}

QRect FormCanvas::formChromeRect() const {
    QPoint o = bodyOrigin();
    return QRect(o.x(), o.y() - kTitleBarH, m_formW, m_formH + kTitleBarH);
}

void FormCanvas::layoutBody() {
    QPoint o = bodyOrigin();
    m_body->setGeometry(o.x(), o.y(), m_formW, m_formH);
    layoutHandles();
    update();
}

QRect FormCanvas::snapRect(const QRect &r) const {
    if (!m_snapEnabled) return r;
    int g = m_gridSize;
    return QRect(snapTo(r.left(), g),  snapTo(r.top(), g),
                 qMax(g, snapTo(r.width(), g)),
                 qMax(g, snapTo(r.height(), g)));
}
QRect FormCanvas::snapMove(const QRect &r) const {
    if (!m_snapEnabled) return r;
    int g = m_gridSize;
    return QRect(snapTo(r.left(), g), snapTo(r.top(), g), r.width(), r.height());
}

void FormCanvas::resizeEvent(QResizeEvent *) {
    layoutBody();
}

// ─── Painting (chrome only — widgets paint themselves) ──────────────────
//
// Title-bar-less designer view: just the workspace background and a 2-px
// raised 3D bezel around the form body.  The body itself (FormBody) paints
// its own background + dot grid.
//
void FormCanvas::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0x80, 0x80, 0x80));      // VB6 workspace gray

    if (m_path.isEmpty()) {
        p.setPen(QColor(0xd0, 0xd0, 0xd0));
        QFont f("MS Sans Serif", 11);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter,
                   "No form open.\nDouble-click a .frm in the project tree.");
        return;
    }

    // ── 2-px raised 3-D bezel around the form body ─────────────────────
    QRect chrome = formChromeRect();        // == body rect (no title bar)
    QRect bezel  = chrome.adjusted(-2, -2, 1, 1);

    // Outer pixel: white top-left, black bottom-right
    p.setPen(QColor(0xff, 0xff, 0xff));
    p.drawLine(bezel.topLeft(),  QPoint(bezel.right(), bezel.top()));
    p.drawLine(bezel.topLeft(),  QPoint(bezel.left(),  bezel.bottom()));
    p.setPen(QColor(0x00, 0x00, 0x00));
    p.drawLine(QPoint(bezel.right(), bezel.top()), bezel.bottomRight());
    p.drawLine(QPoint(bezel.left(),  bezel.bottom()), bezel.bottomRight());

    // Inner pixel: light face / dark shadow
    QRect inner = bezel.adjusted(1, 1, -1, -1);
    p.setPen(QColor(0xdf, 0xdf, 0xdf));
    p.drawLine(inner.topLeft(),  QPoint(inner.right(), inner.top()));
    p.drawLine(inner.topLeft(),  QPoint(inner.left(),  inner.bottom()));
    p.setPen(QColor(0x80, 0x80, 0x80));
    p.drawLine(QPoint(inner.right(), inner.top()), inner.bottomRight());
    p.drawLine(QPoint(inner.left(),  inner.bottom()), inner.bottomRight());
}

// ─── Drop handling ─────────────────────────────────────────────────────
void FormCanvas::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasFormat("application/x-nexor-widget"))
        e->acceptProposedAction();
}

void FormCanvas::dragMoveEvent(QDragMoveEvent *e) {
    if (e->mimeData()->hasFormat("application/x-nexor-widget"))
        e->acceptProposedAction();
}

void FormCanvas::dropEvent(QDropEvent *e) {
    if (m_path.isEmpty()) return;          // no form open
    QString type = QString::fromUtf8(e->mimeData()->data("application/x-nexor-widget"));
    if (type.isEmpty()) return;
    // Convert from FormCanvas coords to m_body-local coords
    createWidgetAt(type, e->pos() - bodyOrigin());
    e->acceptProposedAction();
}

void FormCanvas::createWidgetAt(const QString &type, const QPoint &bodyCenterPos) {
    if (type.isEmpty()) return;
    QSize ds = WidgetFactory::defaultSize(type);
    QPoint pos(
        qBound(0, bodyCenterPos.x() - ds.width()  / 2, m_formW - ds.width()),
        qBound(0, bodyCenterPos.y() - ds.height() / 2, m_formH - ds.height())
    );
    QWidget *w = WidgetFactory::create(type, m_body);
    if (!w) return;
    w->setGeometry(pos.x(), pos.y(), ds.width(), ds.height());
    w->show();
    w->installEventFilter(this);
    Item it { type, uniqueName(WidgetFactory::namePrefix(type)), QString(), w };
    m_items.append(it);
    selectWidget(w);
    emit modified();
}

// ─── Selection & geometry helpers ──────────────────────────────────────
int FormCanvas::itemIndexFor(QWidget *w) const {
    for (int i = 0; i < m_items.size(); ++i)
        if (m_items[i].widget == w) return i;
    return -1;
}

QString FormCanvas::selectedName() const {
    int i = itemIndexFor(m_selected);
    return i < 0 ? QString() : m_items[i].name;
}

QString FormCanvas::selectedType() const {
    int i = itemIndexFor(m_selected);
    return i < 0 ? QString() : m_items[i].type;
}

QString FormCanvas::uniqueName(const QString &prefix) {
    QSet<QString> used;
    for (const Item &it : m_items) used.insert(it.name);
    int n = 1;
    while (used.contains(prefix + QString::number(n))) ++n;
    return prefix + QString::number(n);
}

void FormCanvas::selectWidget(QWidget *w) {
    if (m_selected == w && !m_formSelected) {
        layoutHandles();
        return;
    }
    m_selected = w;
    m_formSelected = false;
    layoutHandles();
    update();
    emit selectionChanged(w);
}

void FormCanvas::selectForm() {
    // Don't enter "form selected" state if no form is open — handles would
    // appear around an invisible body and the property panel would think
    // it's editing a non-existent form.
    if (m_path.isEmpty()) {
        m_selected = nullptr;
        m_formSelected = false;
        layoutHandles();
        emit selectionChanged(nullptr);
        return;
    }
    m_selected = nullptr;
    m_formSelected = true;
    layoutHandles();
    update();
    emit selectionChanged(nullptr);
    emit formSelected();
}

void FormCanvas::selectByName(const QString &name) {
    for (const Item &it : m_items)
        if (it.name == name) { selectWidget(it.widget); return; }
}

void FormCanvas::reorderItems(const QStringList &names) {
    QVector<Item> reordered;
    reordered.reserve(m_items.size());
    QSet<QString> taken;
    for (const QString &n : names) {
        for (const Item &it : m_items)
            if (it.name == n && !taken.contains(n)) {
                reordered.append(it);
                taken.insert(n);
                break;
            }
    }
    // Append anything the caller didn't list.
    for (const Item &it : m_items)
        if (!taken.contains(it.name)) reordered.append(it);

    m_items = reordered;
    // Apply to Qt's focus chain
    for (int i = 1; i < m_items.size(); ++i)
        QWidget::setTabOrder(m_items[i - 1].widget, m_items[i].widget);
    emit modified();
}

void FormCanvas::deleteSelected() {
    if (!m_selected) return;
    int i = itemIndexFor(m_selected);
    if (i < 0) return;
    m_items[i].widget->deleteLater();
    m_items.removeAt(i);
    m_selected = nullptr;
    layoutHandles();
    update();
    emit selectionChanged(nullptr);
    emit modified();
}

void FormCanvas::layoutHandles() {
    // Show handles around either the selected widget (body coords) or the
    // form body itself (when the form is the selection target).
    if (!m_selected && !m_formSelected) {
        for (auto *h : m_handles) h->hide();
        return;
    }
    QRect cg;
    if (m_selected) {
        QRect g = m_selected->geometry();
        QPoint o = bodyOrigin();
        cg = QRect(g.x() + o.x(), g.y() + o.y(), g.width(), g.height());
    } else {
        QPoint o = bodyOrigin();
        cg = QRect(o, QSize(m_formW, m_formH));
    }

    auto place = [&](SelHandle::Dir d, int cx, int cy) {
        m_handles[d]->move(cx - 3, cy - 3);    // handles are 7×7, centred
        m_handles[d]->show();
        m_handles[d]->raise();
    };
    place(SelHandle::TL, cg.left(),     cg.top());
    place(SelHandle::T,  cg.center().x(), cg.top());
    place(SelHandle::TR, cg.right(),    cg.top());
    place(SelHandle::R,  cg.right(),    cg.center().y());
    place(SelHandle::BR, cg.right(),    cg.bottom());
    place(SelHandle::B,  cg.center().x(), cg.bottom());
    place(SelHandle::BL, cg.left(),     cg.bottom());
    place(SelHandle::L,  cg.left(),     cg.center().y());
}

// ─── Property-panel mutators ───────────────────────────────────────────
void FormCanvas::setNameForSelected(const QString &n) {
    int i = itemIndexFor(m_selected);
    if (i < 0 || n.isEmpty()) return;
    m_items[i].name = n;
    emit modified();
}

void FormCanvas::setTextForSelected(const QString &t) {
    if (!m_selected) return;
    WidgetFactory::applyProperty(m_selected, "text", t);
    layoutHandles();           // text may resize button etc.
    emit modified();
}

void FormCanvas::setGeometryForSelected(const QRect &g) {
    if (!m_selected) return;
    QRect bound(0, 0, m_formW - 1, m_formH - 1);
    QRect clip = g.intersected(QRect(0, 0, m_formW, m_formH));
    if (clip.width() < 4)  clip.setWidth(4);
    if (clip.height() < 4) clip.setHeight(4);
    m_selected->setGeometry(clip);
    layoutHandles();
    emit modified();
}

void FormCanvas::setFormTitle(const QString &t) {
    m_title = t; update(); emit modified();
}

void FormCanvas::setDataSource(const QString &s) {
    m_dataSource = s.trimmed();
    emit modified();
}

void FormCanvas::setBindingForSelected(const QString &fieldName) {
    int i = itemIndexFor(m_selected);
    if (i < 0) return;
    m_items[i].binding = fieldName.trimmed();
    emit modified();
}

QString FormCanvas::bindingForSelected() const {
    int i = itemIndexFor(const_cast<FormCanvas*>(this)->m_selected);
    return i < 0 ? QString() : m_items[i].binding;
}

void FormCanvas::setFormSize(const QSize &s) {
    m_formW = qMax(80, s.width());
    m_formH = qMax(60, s.height());
    layoutBody();
    updateGeometry();
    emit modified();
}

void FormCanvas::setFormGeometryFromPanel(const QRect &g) {
    setFormSize(g.size());
}

void FormCanvas::setFormForeground(const QColor &c) {
    m_formFg = c;
    if (!m_body) return;
    // Apply via palette so child widgets that don't set their own foreground
    // pick this up.
    QPalette p = m_body->palette();
    p.setColor(QPalette::WindowText, c.isValid() ? c : QColor(Qt::black));
    p.setColor(QPalette::Text,       c.isValid() ? c : QColor(Qt::black));
    m_body->setPalette(p);
    update();
    emit modified();
}

void FormCanvas::setFormBackground(const QColor &c) {
    m_formBg = c;
    if (m_body) m_body->setBg(c);
    update();
    emit modified();
}

void FormCanvas::setForegroundForSelected(const QColor &c) {
    if (!m_selected) return;
    WidgetFactory::applyProperty(m_selected, "fgColor", c);
    emit modified();
}

void FormCanvas::setBackgroundForSelected(const QColor &c) {
    if (!m_selected) return;
    WidgetFactory::applyProperty(m_selected, "bgColor", c);
    emit modified();
}

void FormCanvas::setVisibleForSelected(bool visible) {
    if (!m_selected) return;
    WidgetFactory::applyProperty(m_selected, "visible", visible);
    // Reflect at design-time as a faint dashed outline if hidden.
    m_selected->setWindowOpacity(visible ? 1.0 : 0.5);
    emit modified();
}

void FormCanvas::setAnchorForSelected(const QString &anchor) {
    if (!m_selected) return;
    WidgetFactory::applyProperty(m_selected, "anchor", anchor);
    emit modified();
}

// ─── Mouse handling ────────────────────────────────────────────────────
void FormCanvas::mousePressEvent(QMouseEvent *e) {
    // Click on canvas outside the body — leave form selected.
    selectForm();
    setFocus(Qt::MouseFocusReason);
    QWidget::mousePressEvent(e);
}

bool FormCanvas::eventFilter(QObject *obj, QEvent *event) {
    // ── Drops & clicks on the form body ──────────────────────────────
    if (obj == m_body) {
        switch (event->type()) {
        case QEvent::DragEnter:
        case QEvent::DragMove: {
            auto *de = static_cast<QDragMoveEvent*>(event);
            if (!m_path.isEmpty()
             && de->mimeData()->hasFormat("application/x-nexor-widget")) {
                de->acceptProposedAction();
                return true;
            }
            return false;
        }
        case QEvent::Drop: {
            auto *de = static_cast<QDropEvent*>(event);
            if (m_path.isEmpty()) return false;
            if (!de->mimeData()->hasFormat("application/x-nexor-widget")) return false;
            QString type = QString::fromUtf8(
                de->mimeData()->data("application/x-nexor-widget"));
            // de->pos() is already in m_body coords
            createWidgetAt(type, de->pos());
            de->acceptProposedAction();
            return true;
        }
        case QEvent::MouseButtonPress:
            selectForm();                      // body click selects the form
            setFocus(Qt::MouseFocusReason);
            return false;
        case QEvent::MouseButtonDblClick:
            selectForm();
            emit formDoubleClicked();
            return true;
        default:
            return false;
        }
    }

    // ── Selection handle drag (resize widget OR form) ────────────────
    for (auto *h : m_handles) {
        if (obj == h) {
            auto *me = static_cast<QMouseEvent*>(event);
            switch (event->type()) {
            case QEvent::MouseButtonPress:
                if (me->button() == Qt::LeftButton
                    && (m_selected || m_formSelected)) {
                    m_op          = OpResize;
                    m_resizeDir   = h->direction();
                    m_pressGlobal = me->globalPos();
                    m_pressGeom   = m_selected
                        ? m_selected->geometry()
                        : QRect(0, 0, m_formW, m_formH);  // form: size only
                }
                return true;
            case QEvent::MouseMove:
                if (m_op == OpResize) {
                    QPoint d = me->globalPos() - m_pressGlobal;
                    using D = SelHandle::Dir;
                    D dir = static_cast<D>(m_resizeDir);

                    if (m_formSelected && !m_selected) {
                        // ── Resize the FORM (centered, so just W/H change)
                        int newW = m_pressGeom.width();
                        int newH = m_pressGeom.height();
                        if (dir == D::R || dir == D::TR || dir == D::BR) newW += d.x();
                        if (dir == D::L || dir == D::TL || dir == D::BL) newW -= d.x();
                        if (dir == D::B || dir == D::BL || dir == D::BR) newH += d.y();
                        if (dir == D::T || dir == D::TL || dir == D::TR) newH -= d.y();
                        if (newW < 80) newW = 80;
                        if (newH < 60) newH = 60;
                        if (!(me->modifiers() & Qt::ControlModifier)) {
                            newW = snapTo(newW, m_gridSize);
                            newH = snapTo(newH, m_gridSize);
                        }
                        setFormSize(QSize(newW, newH));
                    }
                    else if (m_selected) {
                        // ── Resize the selected WIDGET ──────────────
                        QRect g = m_pressGeom;
                        switch (dir) {
                        case D::TL: g.setTopLeft(g.topLeft() + d);             break;
                        case D::T : g.setTop(g.top() + d.y());                 break;
                        case D::TR: g.setTopRight(g.topRight() + d);           break;
                        case D::R : g.setRight(g.right() + d.x());             break;
                        case D::BR: g.setBottomRight(g.bottomRight() + d);     break;
                        case D::B : g.setBottom(g.bottom() + d.y());           break;
                        case D::BL: g.setBottomLeft(g.bottomLeft() + d);       break;
                        case D::L : g.setLeft(g.left() + d.x());               break;
                        }
                        if (g.width()  < 8) g.setWidth(8);
                        if (g.height() < 8) g.setHeight(8);
                        if (g.left() < 0)   g.moveLeft(0);
                        if (g.top()  < 0)   g.moveTop(0);
                        if (g.right()  > m_formW - 1) g.setRight(m_formW - 1);
                        if (g.bottom() > m_formH - 1) g.setBottom(m_formH - 1);
                        if (!(me->modifiers() & Qt::ControlModifier))
                            g = snapRect(g);
                        m_selected->setGeometry(g);
                        layoutHandles();
                        emit modified();
                    }
                }
                return true;
            case QEvent::MouseButtonRelease:
                m_op = OpNone;
                return true;
            default: break;
            }
            return false;
        }
    }

    // ── Designed-widget interception (select / move / dbl-click) ─────
    int idx = itemIndexFor(qobject_cast<QWidget*>(obj));
    if (idx >= 0) {
        QWidget *w  = m_items[idx].widget;
        auto *me    = static_cast<QMouseEvent*>(event);
        switch (event->type()) {
        case QEvent::MouseButtonDblClick:
            selectWidget(w);
            emit widgetDoubleClicked(m_items[idx].name, m_items[idx].type);
            return true;
        case QEvent::MouseButtonPress:
            if (me->button() == Qt::LeftButton) {
                selectWidget(w);
                setFocus(Qt::MouseFocusReason);
                m_op          = OpMove;
                m_pressGlobal = me->globalPos();
                m_pressGeom   = w->geometry();
            }
            return true;
        case QEvent::MouseMove:
            if (m_op == OpMove) {
                QPoint d = me->globalPos() - m_pressGlobal;
                QRect g  = m_pressGeom.translated(d);
                if (g.left() < 0) g.moveLeft(0);
                if (g.top()  < 0) g.moveTop(0);
                if (g.right()  > m_formW - 1) g.moveRight(m_formW - 1);
                if (g.bottom() > m_formH - 1) g.moveBottom(m_formH - 1);
                if (!(me->modifiers() & Qt::ControlModifier))
                    g = snapMove(g);
                w->setGeometry(g);
                layoutHandles();
                emit modified();
            }
            return true;
        case QEvent::MouseButtonRelease:
            m_op = OpNone;
            return true;
        default: break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void FormCanvas::keyPressEvent(QKeyEvent *e) {
    if (m_selected && (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace)) {
        deleteSelected();
        return;
    }
    if (m_selected && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right
                    || e->key() == Qt::Key_Up   || e->key() == Qt::Key_Down)) {
        QRect g = m_selected->geometry();
        int step = (e->modifiers() & Qt::ShiftModifier) ? 10 : 1;
        if (e->key() == Qt::Key_Left)  g.translate(-step, 0);
        if (e->key() == Qt::Key_Right) g.translate( step, 0);
        if (e->key() == Qt::Key_Up)    g.translate(0, -step);
        if (e->key() == Qt::Key_Down)  g.translate(0,  step);
        if (g.left() < 0) g.moveLeft(0);
        if (g.top()  < 0) g.moveTop(0);
        if (g.right()  > m_formW - 1) g.moveRight(m_formW - 1);
        if (g.bottom() > m_formH - 1) g.moveBottom(m_formH - 1);
        m_selected->setGeometry(g);
        layoutHandles();
        emit modified();
        return;
    }
    QWidget::keyPressEvent(e);
}

// ─── Persistence ────────────────────────────────────────────────────────
void FormCanvas::cleanupAllWidgets() {
    for (Item &it : m_items) {
        if (it.widget) it.widget->deleteLater();
    }
    m_items.clear();
    m_selected = nullptr;
    layoutHandles();
}

void FormCanvas::clearForm() {
    cleanupAllWidgets();
    m_path.clear(); m_id.clear(); m_title.clear(); m_code.clear();
    m_dataSource.clear();
    m_formW = 640; m_formH = 480;
    m_formFg = QColor(); m_formBg = QColor();
    m_formSelected = false;
    if (m_body) {
        m_body->setBg(QColor());      // back to default form gray
        m_body->hide();               // no form loaded any more
    }
    layoutBody();
    update();
}

bool FormCanvas::loadForm(const QString &filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        clearForm();
        return false;
    }
    cleanupAllWidgets();
    m_path = filePath;
    m_id.clear(); m_title.clear(); m_code.clear(); m_dataSource.clear();
    m_formW = 640; m_formH = 480;

    QXmlStreamReader r(&f);

    // Per-Widget scratch + properties accumulator.
    bool inWidget = false;
    bool inFormProps = false;     // <Property> directly under <Form>
    QString cType, cName;
    int cx = 0, cy = 0, cw = 0, ch = 0;
    QHash<QString, QString> cProps;     // widget properties
    QHash<QString, QString> formProps;  // form-level properties

    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement()) {
            const QStringRef n = r.name();
            if (n == "Form") {
                const auto a = r.attributes();
                m_id         = a.value("id").toString();
                m_dataSource = a.value("dataSource").toString();
                inFormProps = true;
            } else if (n == "Geometry") {
                const auto a = r.attributes();
                if (a.hasAttribute("width"))  m_formW = a.value("width").toInt();
                if (a.hasAttribute("height")) m_formH = a.value("height").toInt();
            } else if (n == "Title") {
                m_title = r.readElementText();
            } else if (n == "Code") {
                m_code = r.readElementText();
            } else if (n == "Widget") {
                inWidget = true;
                inFormProps = false;
                cProps.clear();
                const auto a = r.attributes();
                cType = a.value("type").toString();
                cName = a.value("name").toString();
                cx = a.value("x").toInt();
                cy = a.value("y").toInt();
                cw = a.value("width").toInt();
                ch = a.value("height").toInt();
                cProps["__binding"] = a.value("binding").toString();
            } else if (n == "Property") {
                QString pn = r.attributes().value("name").toString();
                QString pv = r.readElementText();
                if (inWidget)         cProps[pn] = pv;
                else if (inFormProps) formProps[pn] = pv;
            }
        } else if (r.isEndElement()) {
            if (r.name() == "Widget" && inWidget) {
                if (QWidget *w = WidgetFactory::create(cType, m_body)) {
                    if (cw < 4) cw = WidgetFactory::defaultSize(cType).width();
                    if (ch < 4) ch = WidgetFactory::defaultSize(cType).height();
                    w->setGeometry(cx, cy, cw, ch);
                    // Apply every saved property through the factory.
                    for (auto it = cProps.begin(); it != cProps.end(); ++it) {
                        QString key = it.key(), val = it.value();
                        if (key == "fgColor" || key == "bgColor")
                            WidgetFactory::applyProperty(w, key, QColor(val));
                        else if (key == "visible")
                            WidgetFactory::applyProperty(w, key, val == "true");
                        else
                            WidgetFactory::applyProperty(w, key, val);
                    }
                    w->show();
                    w->installEventFilter(this);
                    Item it { cType,
                              cName.isEmpty()
                                ? uniqueName(WidgetFactory::namePrefix(cType))
                                : cName,
                              cProps.value("__binding"),
                              w };
                    m_items.append(it);
                }
                inWidget = false;
            }
        }
    }

    // Apply form-level properties.
    if (formProps.contains("fgColor")) m_formFg = QColor(formProps["fgColor"]);
    if (formProps.contains("bgColor")) m_formBg = QColor(formProps["bgColor"]);
    if (m_formFg.isValid() || m_formBg.isValid())
        setFormForeground(m_formFg);   // also re-applies bg

    if (m_title.isEmpty()) m_title = m_id;
    if (m_body) m_body->show();         // form is now loaded → show body
    layoutBody();
    update();
    selectForm();                       // start with form selected
    return !r.hasError();
}

bool FormCanvas::saveForm() {
    if (m_path.isEmpty()) return false;
    QFile f(m_path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QXmlStreamWriter w(&f);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("Form");
    w.writeAttribute("version", "1");
    w.writeAttribute("id", m_id);
    if (!m_dataSource.isEmpty())
        w.writeAttribute("dataSource", m_dataSource);

    w.writeStartElement("Geometry");
    w.writeAttribute("x", "100");
    w.writeAttribute("y", "100");
    w.writeAttribute("width",  QString::number(m_formW));
    w.writeAttribute("height", QString::number(m_formH));
    w.writeEndElement();

    w.writeStartElement("Title");
    w.writeCharacters(m_title);
    w.writeEndElement();

    // Form-level properties (only fgColor/bgColor for now — geometry already
    // lives in <Geometry>; visible/anchor don't apply to the form).
    auto writeProp = [&](const QString &name, const QString &value) {
        w.writeStartElement("Property");
        w.writeAttribute("name", name);
        w.writeCharacters(value);
        w.writeEndElement();
    };
    if (m_formFg.isValid()) writeProp("fgColor", m_formFg.name());
    if (m_formBg.isValid()) writeProp("bgColor", m_formBg.name());

    w.writeStartElement("Widgets");
    for (const Item &it : m_items) {
        w.writeStartElement("Widget");
        w.writeAttribute("type",   it.type);
        w.writeAttribute("name",   it.name);
        w.writeAttribute("x",      QString::number(it.widget->x()));
        w.writeAttribute("y",      QString::number(it.widget->y()));
        w.writeAttribute("width",  QString::number(it.widget->width()));
        w.writeAttribute("height", QString::number(it.widget->height()));
        if (!it.binding.isEmpty())
            w.writeAttribute("binding", it.binding);

        if (WidgetFactory::hasTextProperty(it.type))
            writeProp("text", WidgetFactory::readProperty(it.widget, "text").toString());

        QColor fg = WidgetFactory::readProperty(it.widget, "fgColor").value<QColor>();
        if (fg.isValid()) writeProp("fgColor", fg.name());
        QColor bg = WidgetFactory::readProperty(it.widget, "bgColor").value<QColor>();
        if (bg.isValid()) writeProp("bgColor", bg.name());

        QVariant visV = WidgetFactory::readProperty(it.widget, "visible");
        if (visV.isValid()) writeProp("visible", visV.toBool() ? "true" : "false");

        QString anchor = WidgetFactory::readProperty(it.widget, "anchor").toString();
        if (!anchor.isEmpty()) writeProp("anchor", anchor);

        w.writeEndElement();
    }
    w.writeEndElement();

    // Form-level event-driven code (Form_Load, btnX_Click, ...)
    w.writeStartElement("Code");
    w.writeCDATA(m_code);
    w.writeEndElement();

    w.writeEndElement(); // Form
    w.writeEndDocument();
    return true;
}
