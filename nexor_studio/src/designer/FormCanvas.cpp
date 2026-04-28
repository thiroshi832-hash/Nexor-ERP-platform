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

// =============================================================================
// FormCanvas::SelHandle — small overlay widget at one of 8 positions around
// the currently-selected designed widget.  FormCanvas installs an event
// filter on each handle to drive the resize.
// =============================================================================
class FormCanvas::SelHandle : public QWidget {
public:
    enum Dir { TL = 0, T, TR, R, BR, B, BL, L };

    SelHandle(Dir d, QWidget *parent) : QWidget(parent), m_dir(d) {
        setFixedSize(8, 8);
        setStyleSheet("background:#5b8cff; border:1px solid #ffffff;");
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
// FormCanvas
// =============================================================================
FormCanvas::FormCanvas(QWidget *parent) : QWidget(parent) {
    setObjectName("formCanvas");
    setStyleSheet("QWidget#formCanvas { background:#2d2d30; }");
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    setMouseTracking(true);

    // Body: the form's client area where designed widgets live.
    m_body = new QWidget(this);
    m_body->setObjectName("formBody");
    m_body->setAttribute(Qt::WA_StyledBackground);
    m_body->setStyleSheet(
        "QWidget#formBody { background:#f5f5f5; border:1px solid #1e2030; }");
    m_body->setMouseTracking(true);
    m_body->setAcceptDrops(true);          // accept palette drops on body
    m_body->installEventFilter(this);
    m_body->resize(m_formW, m_formH);

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

void FormCanvas::resizeEvent(QResizeEvent *) {
    layoutBody();
}

// ─── Painting (chrome only — widgets paint themselves) ──────────────────
void FormCanvas::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Dotted grid
    p.fillRect(rect(), QColor(0x2d, 0x2d, 0x30));
    p.setPen(QColor(0x37, 0x37, 0x3a));
    for (int x = 0; x < width();  x += 24) p.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += 24) p.drawLine(0, y, width(), y);

    if (m_path.isEmpty()) {
        p.setPen(QColor(0x80, 0x80, 0x80));
        QFont f = p.font(); f.setPointSize(13); f.setWeight(QFont::Light);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter,
                   "No form open.\nDouble-click a .frm in the project tree.");
        return;
    }

    // Form chrome: shadow + title bar above body
    QRect chrome = formChromeRect();
    p.fillRect(chrome.adjusted(6, 8, 6, 8), QColor(0, 0, 0, 110)); // shadow

    QRect titleBar(chrome.left(), chrome.top(), chrome.width(), kTitleBarH);
    p.fillRect(titleBar, QColor(0x35, 0x39, 0x44));

    QFont tf = p.font();
    tf.setPointSize(10); tf.setWeight(QFont::DemiBold);
    p.setFont(tf);
    p.setPen(QColor(0xdc, 0xe1, 0xe7));
    p.drawText(titleBar.adjusted(12, 0, -90, 0),
               Qt::AlignVCenter | Qt::AlignLeft, m_title);

    int cx = titleBar.right() - 12;
    auto drawBtn = [&](const QString &g) {
        int w = 22;
        QRect r(cx - w, titleBar.top(), w, titleBar.height());
        p.drawText(r, Qt::AlignCenter, g);
        cx -= w;
    };
    drawBtn(QStringLiteral("✕"));
    drawBtn(QStringLiteral("□"));
    drawBtn(QStringLiteral("–"));

    // Footer
    QRect footer(0, height() - 24, width(), 24);
    p.fillRect(footer, QColor(0x1e, 0x20, 0x26));
    p.setPen(QColor(0x9a, 0x9a, 0x9a));
    QFont ff = p.font(); ff.setPointSize(9); ff.setWeight(QFont::Normal);
    p.setFont(ff);
    QString left  = QString("  %1").arg(m_path);
    QString right = QString("%1 × %2  ·  %3 widgets  ")
                      .arg(m_formW).arg(m_formH).arg(m_items.size());
    QFontMetrics fm(ff);
    int rw = fm.horizontalAdvance(right);
    p.drawText(footer.adjusted(0, 0, -rw, 0), Qt::AlignVCenter | Qt::AlignLeft,
               fm.elidedText(left, Qt::ElideMiddle, footer.width() - rw - 12));
    p.drawText(footer.adjusted(footer.width() - rw, 0, 0, 0),
               Qt::AlignVCenter | Qt::AlignLeft, right);
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
    Item it { type, uniqueName(WidgetFactory::namePrefix(type)), w };
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
    if (m_selected == w) {
        layoutHandles();
        return;
    }
    m_selected = w;
    layoutHandles();
    update();
    emit selectionChanged(w);
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
    bool show = (m_selected != nullptr);
    if (!show) {
        for (auto *h : m_handles) h->hide();
        return;
    }
    // Map selected widget geometry (body coords) → canvas coords
    QRect g = m_selected->geometry();
    QPoint o = bodyOrigin();
    QRect cg(g.x() + o.x(), g.y() + o.y(), g.width(), g.height());

    auto place = [&](SelHandle::Dir d, int cx, int cy) {
        m_handles[d]->move(cx - 4, cy - 4);
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

void FormCanvas::setFormSize(const QSize &s) {
    m_formW = qMax(80, s.width());
    m_formH = qMax(60, s.height());
    layoutBody();
    updateGeometry();
    emit modified();
}

// ─── Mouse handling ────────────────────────────────────────────────────
void FormCanvas::mousePressEvent(QMouseEvent *e) {
    // Click on canvas (outside body) deselects
    selectWidget(nullptr);
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
            selectWidget(nullptr);
            setFocus(Qt::MouseFocusReason);
            return false;
        default:
            return false;
        }
    }

    // ── Selection handle drag (resize) ───────────────────────────────
    for (auto *h : m_handles) {
        if (obj == h) {
            auto *me = static_cast<QMouseEvent*>(event);
            switch (event->type()) {
            case QEvent::MouseButtonPress:
                if (me->button() == Qt::LeftButton && m_selected) {
                    m_op          = OpResize;
                    m_resizeDir   = h->direction();
                    m_pressGlobal = me->globalPos();
                    m_pressGeom   = m_selected->geometry();
                }
                return true;
            case QEvent::MouseMove:
                if (m_op == OpResize && m_selected) {
                    QPoint d = me->globalPos() - m_pressGlobal;
                    QRect g  = m_pressGeom;
                    using D = SelHandle::Dir;
                    switch (static_cast<D>(m_resizeDir)) {
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
                    m_selected->setGeometry(g);
                    layoutHandles();
                    emit modified();
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

    // ── Designed-widget interception (select / move) ─────────────────
    int idx = itemIndexFor(qobject_cast<QWidget*>(obj));
    if (idx >= 0) {
        QWidget *w  = m_items[idx].widget;
        auto *me    = static_cast<QMouseEvent*>(event);
        switch (event->type()) {
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
    m_formW = 640; m_formH = 480;
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
    m_id.clear(); m_title.clear(); m_code.clear();
    m_formW = 640; m_formH = 480;

    QXmlStreamReader r(&f);

    // Per-Widget scratch
    bool inWidget = false;
    QString cType, cName, cText;
    int cx = 0, cy = 0, cw = 0, ch = 0;

    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement()) {
            const QStringRef n = r.name();
            if (n == "Form") {
                m_id = r.attributes().value("id").toString();
            } else if (n == "Geometry") {
                const auto a = r.attributes();
                if (a.hasAttribute("width"))  m_formW = a.value("width").toInt();
                if (a.hasAttribute("height")) m_formH = a.value("height").toInt();
            } else if (n == "Title") {
                m_title = r.readElementText();
            } else if (n == "Code") {
                m_code = r.readElementText();
            } else if (n == "Widget") {
                inWidget = true; cText.clear();
                const auto a = r.attributes();
                cType = a.value("type").toString();
                cName = a.value("name").toString();
                cx = a.value("x").toInt();
                cy = a.value("y").toInt();
                cw = a.value("width").toInt();
                ch = a.value("height").toInt();
            } else if (n == "Property" && inWidget) {
                QString pn = r.attributes().value("name").toString();
                QString pv = r.readElementText();
                if (pn == "text") cText = pv;
            }
        } else if (r.isEndElement()) {
            if (r.name() == "Widget" && inWidget) {
                if (QWidget *w = WidgetFactory::create(cType, m_body)) {
                    if (cw < 4) cw = WidgetFactory::defaultSize(cType).width();
                    if (ch < 4) ch = WidgetFactory::defaultSize(cType).height();
                    w->setGeometry(cx, cy, cw, ch);
                    if (!cText.isEmpty())
                        WidgetFactory::applyProperty(w, "text", cText);
                    w->show();
                    w->installEventFilter(this);
                    Item it { cType,
                              cName.isEmpty()
                                ? uniqueName(WidgetFactory::namePrefix(cType))
                                : cName,
                              w };
                    m_items.append(it);
                }
                inWidget = false;
            }
        }
    }

    if (m_title.isEmpty()) m_title = m_id;
    layoutBody();
    update();
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

    w.writeStartElement("Geometry");
    w.writeAttribute("x", "100");
    w.writeAttribute("y", "100");
    w.writeAttribute("width",  QString::number(m_formW));
    w.writeAttribute("height", QString::number(m_formH));
    w.writeEndElement();

    w.writeStartElement("Title");
    w.writeCharacters(m_title);
    w.writeEndElement();

    w.writeStartElement("Widgets");
    for (const Item &it : m_items) {
        w.writeStartElement("Widget");
        w.writeAttribute("type",   it.type);
        w.writeAttribute("name",   it.name);
        w.writeAttribute("x",      QString::number(it.widget->x()));
        w.writeAttribute("y",      QString::number(it.widget->y()));
        w.writeAttribute("width",  QString::number(it.widget->width()));
        w.writeAttribute("height", QString::number(it.widget->height()));
        if (WidgetFactory::hasTextProperty(it.type)) {
            QString text = WidgetFactory::readProperty(it.widget, "text").toString();
            w.writeStartElement("Property");
            w.writeAttribute("name", "text");
            w.writeCharacters(text);
            w.writeEndElement();
        }
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
