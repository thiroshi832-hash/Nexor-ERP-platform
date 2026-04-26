#include "FancyTabBar.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QFont>

// =============================================================================
// FancyTab
// =============================================================================
FancyTab::FancyTab(int mode, const QString &iconKind, const QString &label, QWidget *parent)
    : QWidget(parent)
    , m_mode(mode)
    , m_iconKind(iconKind)
    , m_label(label) {
    setFixedHeight(72);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
}

void FancyTab::setActive(bool a) {
    if (m_active == a) return;
    m_active = a;
    update();
}

QSize FancyTab::sizeHint() const { return QSize(72, 72); }

void FancyTab::enterEvent(QEvent *) { m_hover = true;  update(); }
void FancyTab::leaveEvent(QEvent *) { m_hover = false; update(); }

void FancyTab::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) emit clicked(m_mode);
}

void FancyTab::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    QColor bg(0x0d, 0x0e, 0x12);
    if (m_active)      bg = QColor(0x18, 0x1a, 0x22);
    else if (m_hover)  bg = QColor(0x14, 0x16, 0x1c);
    p.fillRect(rect(), bg);

    // Accent stripe on the left when active
    if (m_active) {
        QRect stripe(0, 0, 3, height());
        p.fillRect(stripe, QColor(0x5b, 0x8c, 0xff));
    }

    // Foreground colour
    QColor fg = m_active ? QColor(0x5b, 0x8c, 0xff)
              : m_hover  ? QColor(0xdc, 0xe1, 0xe7)
                         : QColor(0x8a, 0x95, 0xa3);

    // Icon area
    QRect iconRect(0, 8, width(), 36);
    drawIcon(p, iconRect, fg);

    // Label area
    QRect textRect(0, 46, width(), 22);
    QFont f = p.font();
    f.setFamily("Segoe UI");
    f.setPointSizeF(7.5);
    f.setWeight(QFont::DemiBold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    p.setFont(f);
    p.setPen(fg);
    p.drawText(textRect, Qt::AlignCenter, m_label);
}

void FancyTab::drawIcon(QPainter &p, const QRect &r, const QColor &c) {
    const int sz = 22;
    int cx = r.center().x();
    int cy = r.center().y();
    QRect b(cx - sz/2, cy - sz/2, sz, sz);

    QPen pen(c, 1.6);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    if (m_iconKind == "welcome") {
        // House: triangle roof + walls + door
        QPainterPath path;
        path.moveTo(b.center().x(), b.top());
        path.lineTo(b.right(),      b.center().y());
        path.lineTo(b.right() - 2,  b.center().y());
        path.lineTo(b.right() - 2,  b.bottom());
        path.lineTo(b.left()  + 2,  b.bottom());
        path.lineTo(b.left()  + 2,  b.center().y());
        path.lineTo(b.left(),       b.center().y());
        path.closeSubpath();
        p.drawPath(path);
        QRect door(b.center().x() - 3, b.bottom() - 7, 6, 7);
        p.drawRect(door);

    } else if (m_iconKind == "edit") {
        // Pencil running diagonally
        QPainterPath body;
        body.moveTo(b.left() + 2,  b.bottom() - 2);
        body.lineTo(b.left() + 6,  b.bottom() - 6);
        body.lineTo(b.right() - 4, b.top() + 2);
        body.lineTo(b.right(),     b.top() + 6);
        body.lineTo(b.left() + 6,  b.bottom() - 2);
        body.closeSubpath();
        p.drawPath(body);
        p.drawLine(b.left() + 2, b.bottom() - 2, b.left() + 5, b.bottom() - 5);

    } else if (m_iconKind == "design") {
        // Square with divider crosshair
        p.drawRoundedRect(b.adjusted(0, 0, -1, -1), 3, 3);
        p.drawLine(b.topLeft(),     b.bottomRight());
        p.drawLine(b.center().x(),  b.top(),       b.center().x(),  b.bottom());
        p.drawLine(b.left(),        b.center().y(), b.right(),       b.center().y());

    } else if (m_iconKind == "debug") {
        // Bug body + antennae + legs
        QRect body(b.left() + 4, b.top() + 6, b.width() - 8, b.height() - 10);
        p.drawRoundedRect(body, body.width()/2, body.height()/3);
        p.drawLine(body.left()  + 2, body.top(), b.left()  + 2, b.top());
        p.drawLine(body.right() - 2, body.top(), b.right() - 2, b.top());
        p.drawLine(body.left(),  body.center().y() - 3, b.left(),  body.center().y() - 3);
        p.drawLine(body.left(),  body.center().y() + 3, b.left(),  body.center().y() + 3);
        p.drawLine(body.right(), body.center().y() - 3, b.right(), body.center().y() - 3);
        p.drawLine(body.right(), body.center().y() + 3, b.right(), body.center().y() + 3);

    } else if (m_iconKind == "projects") {
        // Folder
        QPainterPath f;
        f.moveTo(b.left(),      b.top() + 4);
        f.lineTo(b.left() + 7,  b.top() + 4);
        f.lineTo(b.left() + 9,  b.top() + 7);
        f.lineTo(b.right(),     b.top() + 7);
        f.lineTo(b.right(),     b.bottom() - 1);
        f.lineTo(b.left(),      b.bottom() - 1);
        f.closeSubpath();
        p.drawPath(f);

    } else if (m_iconKind == "help") {
        // ? in a circle
        p.drawEllipse(b);
        QFont qf = p.font();
        qf.setPointSizeF(13);
        qf.setBold(true);
        p.setFont(qf);
        p.drawText(b, Qt::AlignCenter, "?");
    }
}

// =============================================================================
// FancyTabBar
// =============================================================================
FancyTabBar::FancyTabBar(QWidget *parent) : QWidget(parent) {
    setFixedWidth(72);
    setStyleSheet("FancyTabBar { background:#0d0e12;"
                  " border-right:1px solid #1e2030; }");

    auto *col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    addTab(ModeWelcome,  "welcome",  "WELCOME");
    addTab(ModeEdit,     "edit",     "EDIT");
    addTab(ModeDesign,   "design",   "DESIGN");
    addTab(ModeDebug,    "debug",    "DEBUG");
    addTab(ModeProjects, "projects", "PROJECTS");

    for (auto *t : m_tabs) col->addWidget(t);

    col->addStretch();

    addTab(ModeHelp, "help", "HELP");
    col->addWidget(m_tabs.last());

    // Initial state: Welcome active
    m_current = ModeWelcome;
    m_tabs.first()->setActive(true);
}

void FancyTabBar::addTab(int mode, const QString &iconKind, const QString &label) {
    auto *t = new FancyTab(mode, iconKind, label, this);
    connect(t, &FancyTab::clicked, this, [this](int m) { setCurrentMode(m); });
    m_tabs.append(t);
}

void FancyTabBar::setCurrentMode(int mode) {
    if (mode == m_current) return;
    for (auto *t : m_tabs) t->setActive(t->mode() == mode);
    m_current = mode;
    emit currentChanged(mode);
}
