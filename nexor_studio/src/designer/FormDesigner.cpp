#include "FormDesigner.h"

#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QXmlStreamReader>
#include <QFontMetrics>

FormDesigner::FormDesigner(QWidget *parent) : QWidget(parent) {
    setStyleSheet("background:#2d2d30;");
    setMinimumSize(400, 300);
}

bool FormDesigner::loadForm(const QString &filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        clearForm();
        return false;
    }
    QXmlStreamReader r(&f);
    m_id.clear(); m_title.clear();
    m_x = 100; m_y = 100; m_w = 640; m_h = 480;

    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement()) continue;
        const auto name = r.name();
        if (name == "Form") {
            m_id = r.attributes().value("id").toString();
        } else if (name == "Geometry") {
            const auto a = r.attributes();
            m_x = a.value("x").toInt();
            m_y = a.value("y").toInt();
            m_w = a.value("width").toInt();
            m_h = a.value("height").toInt();
        } else if (name == "Title") {
            m_title = r.readElementText();
        }
    }

    if (m_w <= 0) m_w = 640;
    if (m_h <= 0) m_h = 480;
    if (m_title.isEmpty()) m_title = m_id;
    m_path   = filePath;
    m_loaded = true;
    update();
    return !r.hasError();
}

void FormDesigner::clearForm() {
    m_path.clear(); m_id.clear(); m_title.clear();
    m_loaded = false;
    update();
}

void FormDesigner::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Canvas background grid (Qt-Designer style)
    p.fillRect(rect(), QColor(0x2d, 0x2d, 0x30));
    QPen grid(QColor(0x37, 0x37, 0x3a));
    grid.setWidth(1);
    p.setPen(grid);
    for (int x = 0; x < width();  x += 24) p.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += 24) p.drawLine(0, y, width(), y);

    if (!m_loaded) {
        p.setPen(QColor(0x80, 0x80, 0x80));
        QFont f = p.font(); f.setPointSize(13); f.setWeight(QFont::Light);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter,
                   "No form open.\nDouble-click a .frm in the project tree.");
        return;
    }

    // ── Form preview rectangle (scaled to fit canvas with margin) ─────────
    const int margin = 40;
    QSize avail(width() - 2 * margin, height() - 2 * margin - 40);  // 40 = footer
    double sx = double(avail.width())  / m_w;
    double sy = double(avail.height()) / m_h;
    double scale = qMin(1.0, qMin(sx, sy));
    int fw = int(m_w * scale);
    int fh = int(m_h * scale);
    QRect formRect((width() - fw) / 2, margin + (avail.height() - fh) / 2,
                   fw, fh);

    // Drop shadow
    QPainterPath shadow;
    shadow.addRect(formRect.adjusted(6, 8, 6, 8));
    p.fillPath(shadow, QColor(0, 0, 0, 110));

    // Form body
    p.fillRect(formRect, QColor(0xee, 0xee, 0xee));

    // Title bar
    QRect titleBar(formRect.left(), formRect.top(), formRect.width(), 28);
    p.fillRect(titleBar, QColor(0x35, 0x39, 0x44));

    // Title text
    QFont tf = p.font();
    tf.setPointSize(10); tf.setWeight(QFont::DemiBold);
    p.setFont(tf);
    p.setPen(QColor(0xdc, 0xe1, 0xe7));
    p.drawText(titleBar.adjusted(12, 0, -90, 0),
               Qt::AlignVCenter | Qt::AlignLeft, m_title);

    // Window controls (- ▢ ×)
    int cx = titleBar.right() - 12;
    p.setPen(QColor(0xdc, 0xe1, 0xe7));
    auto drawBtn = [&](const QString &g) {
        int w = 22;
        QRect r(cx - w, titleBar.top(), w, titleBar.height());
        p.drawText(r, Qt::AlignCenter, g);
        cx -= w;
    };
    drawBtn(QStringLiteral("✕"));  // ×
    drawBtn(QStringLiteral("□"));  // ▢
    drawBtn(QStringLiteral("–"));  // –

    // Body content placeholder
    QRect body(formRect.left(), titleBar.bottom() + 1,
               formRect.width(), formRect.bottom() - titleBar.bottom() - 1);
    p.setPen(QColor(0xa0, 0xa0, 0xa0));
    QFont bf = p.font();
    bf.setPointSize(11); bf.setWeight(QFont::Light);
    p.setFont(bf);
    p.drawText(body, Qt::AlignCenter,
               "Drag widgets here from the palette\n(palette ships in feature/designer)");

    // ── Footer line ───────────────────────────────────────────────────────
    QRect footer(0, height() - 32, width(), 32);
    p.fillRect(footer, QColor(0x1e, 0x20, 0x26));
    p.setPen(QColor(0x9a, 0x9a, 0x9a));
    QFont ff = p.font();
    ff.setPointSize(9); ff.setWeight(QFont::Normal);
    p.setFont(ff);
    QFontMetrics fm(ff);
    QString left  = QString("  %1").arg(m_path);
    QString right = QString("%1 × %2  @ scale %3%  ")
                      .arg(m_w).arg(m_h).arg(int(scale * 100));
    int rw = fm.horizontalAdvance(right);
    p.drawText(footer.adjusted(0, 0, -rw, 0),
               Qt::AlignVCenter | Qt::AlignLeft,
               fm.elidedText(left, Qt::ElideMiddle, footer.width() - rw - 12));
    p.drawText(footer.adjusted(footer.width() - rw, 0, 0, 0),
               Qt::AlignVCenter | Qt::AlignLeft, right);
}
