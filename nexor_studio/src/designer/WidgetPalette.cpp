#include "WidgetPalette.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QMimeData>
#include <QDrag>

// Subclass to encode a custom MIME type (application/x-nexor-widget) carrying
// the widget type string when the user drags an item.
namespace {
class PaletteTree : public QTreeWidget {
public:
    explicit PaletteTree(QWidget *parent = nullptr) : QTreeWidget(parent) {}
protected:
    QMimeData *mimeData(const QList<QTreeWidgetItem*> items) const override {
        if (items.isEmpty() || !items.first()->parent()) return nullptr;
        auto *m = new QMimeData;
        m->setData("application/x-nexor-widget",
                   items.first()->data(0, Qt::UserRole).toString().toUtf8());
        m->setText(items.first()->text(0));   // generic fallback
        return m;
    }
    QStringList mimeTypes() const override {
        return { "application/x-nexor-widget", "text/plain" };
    }
};
} // namespace

WidgetPalette::WidgetPalette(QWidget *parent) : QWidget(parent) {
    setStyleSheet(R"(
        WidgetPalette { background:#1b1d23; border-right:1px solid #1e2030; }
        QLabel#paletteHeader {
            background:#0d0e12; color:#8a95a3;
            padding:8px 12px; border-bottom:1px solid #1e2030;
            font-size:11px; font-weight:600; letter-spacing:2px;
        }
        QTreeWidget {
            background:#1b1d23; color:#dce1e7;
            border:none; outline:0;
            font-family:"Segoe UI"; font-size:12px;
        }
        QTreeWidget::item          { padding:3px 4px; }
        QTreeWidget::item:hover    { background:#243049; }
        QTreeWidget::item:selected { background:#1e3a5f; color:#ffffff; }
        QTreeWidget::branch        { background:#1b1d23; }
    )");

    auto *col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    auto *header = new QLabel("WIDGETS", this);
    header->setObjectName("paletteHeader");
    col->addWidget(header);

    m_tree = new PaletteTree(this);
    m_tree->setHeaderHidden(true);
    m_tree->setIndentation(12);
    m_tree->setUniformRowHeights(true);
    m_tree->setIconSize(QSize(18, 18));
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setDragEnabled(true);
    m_tree->setDragDropMode(QAbstractItemView::DragOnly);
    col->addWidget(m_tree, 1);

    // ── Common Controls ──
    auto *common = addCategory("Common Controls");
    addWidget(common, "Button",       "Clickable button");
    addWidget(common, "Label",        "Static text label");
    addWidget(common, "TextBox",      "Single-line text input");
    addWidget(common, "TextArea",     "Multi-line text input");
    addWidget(common, "CheckBox",     "Toggle on/off");
    addWidget(common, "RadioButton",  "Mutually-exclusive choice");
    addWidget(common, "ComboBox",     "Dropdown selector");
    addWidget(common, "ListBox",      "Scrollable item list");

    // ── Containers ──
    auto *cont = addCategory("Containers");
    addWidget(cont, "GroupBox",   "Box with title");
    addWidget(cont, "Panel",      "Plain container");
    addWidget(cont, "TabControl", "Tabbed container");

    // ── Display ──
    auto *disp = addCategory("Display");
    addWidget(disp, "PictureBox",  "Image display");
    addWidget(disp, "ProgressBar", "Progress indicator");
    addWidget(disp, "Separator",   "Horizontal divider");

    // ── Inputs ──
    auto *inp = addCategory("Inputs");
    addWidget(inp, "Slider",         "Drag to choose value");
    addWidget(inp, "DateTimePicker", "Pick a date or time");
    addWidget(inp, "NumericUpDown",  "Numeric spinner");

    // Expand all categories
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i)
        m_tree->topLevelItem(i)->setExpanded(true);

    connect(m_tree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *it, int) {
        if (it && it->parent())   // leaf items only
            emit widgetSelected(it->data(0, Qt::UserRole).toString());
    });
}

QTreeWidgetItem *WidgetPalette::addCategory(const QString &name) {
    auto *cat = new QTreeWidgetItem(m_tree);
    cat->setText(0, name);
    QFont f = cat->font(0);
    f.setWeight(QFont::DemiBold);
    cat->setFont(0, f);
    cat->setForeground(0, QBrush(QColor("#8a95a3")));
    cat->setFlags(cat->flags() & ~Qt::ItemIsSelectable);
    return cat;
}

void WidgetPalette::addWidget(QTreeWidgetItem *category,
                              const QString &type,
                              const QString &description) {
    auto *it = new QTreeWidgetItem(category);
    it->setText(0, type);
    it->setIcon(0, QIcon(makeIcon(type)));
    it->setData(0, Qt::UserRole, type);
    if (!description.isEmpty())
        it->setToolTip(0, description);
}

// ─── Hand-drawn 18×18 widget icons ────────────────────────────────────────
QPixmap WidgetPalette::makeIcon(const QString &type) const {
    const int sz = 18;
    QPixmap pm(sz, sz);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(QColor(0xb8, 0xc1, 0xcc), 1.2);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    QRect r(1, 3, sz - 2, sz - 6);

    if (type == "Button") {
        p.setBrush(QColor(0x2a, 0x40, 0x66));
        p.drawRoundedRect(r, 3, 3);

    } else if (type == "Label") {
        QFont f = p.font();
        f.setPointSize(11); f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(0xdc, 0xe1, 0xe7));
        p.drawText(pm.rect(), Qt::AlignCenter, "T");

    } else if (type == "TextBox") {
        p.drawRoundedRect(r, 2, 2);
        // text caret
        p.setPen(QPen(QColor(0x5b, 0x8c, 0xff), 1.2));
        p.drawLine(r.left() + 4, r.top() + 3, r.left() + 4, r.bottom() - 3);

    } else if (type == "TextArea") {
        p.drawRoundedRect(r, 2, 2);
        for (int y = r.top() + 4; y < r.bottom() - 1; y += 3)
            p.drawLine(r.left() + 3, y, r.right() - 3, y);

    } else if (type == "CheckBox") {
        QRect box(r.left(), r.top() + 1, r.height() - 2, r.height() - 2);
        p.drawRect(box);
        QPainterPath chk;
        chk.moveTo(box.left() + 2, box.center().y());
        chk.lineTo(box.center().x(), box.bottom() - 2);
        chk.lineTo(box.right() - 1, box.top() + 2);
        QPen cp(QColor(0x5b, 0x8c, 0xff), 1.4);
        p.setPen(cp);
        p.drawPath(chk);

    } else if (type == "RadioButton") {
        QRect circ(r.left(), r.top() + 1, r.height() - 2, r.height() - 2);
        p.drawEllipse(circ);
        QRect dot = circ.adjusted(3, 3, -3, -3);
        p.setBrush(QColor(0x5b, 0x8c, 0xff));
        p.setPen(Qt::NoPen);
        p.drawEllipse(dot);

    } else if (type == "ComboBox") {
        p.drawRoundedRect(r, 2, 2);
        // dropdown arrow
        QPainterPath arr;
        int ax = r.right() - 5, ay = r.center().y() - 1;
        arr.moveTo(ax - 2, ay);
        arr.lineTo(ax + 2, ay);
        arr.lineTo(ax,     ay + 3);
        arr.closeSubpath();
        p.fillPath(arr, QColor(0xdc, 0xe1, 0xe7));

    } else if (type == "ListBox") {
        p.drawRect(r);
        for (int y = r.top() + 3; y < r.bottom() - 1; y += 3)
            p.drawLine(r.left() + 2, y, r.right() - 2, y);

    } else if (type == "GroupBox") {
        QRect g = r.adjusted(0, 1, 0, 0);
        // top edge with gap for label
        p.drawLine(g.left(),     g.top(), g.left() + 3, g.top());
        p.drawLine(g.left() + 8, g.top(), g.right(),    g.top());
        p.drawLine(g.left(),  g.top(),    g.left(),  g.bottom());
        p.drawLine(g.right(), g.top(),    g.right(), g.bottom());
        p.drawLine(g.left(),  g.bottom(), g.right(), g.bottom());

    } else if (type == "Panel") {
        QPen dotted(QColor(0xb8, 0xc1, 0xcc), 1, Qt::DotLine);
        p.setPen(dotted);
        p.drawRect(r);

    } else if (type == "TabControl") {
        // tabs
        QRect t1(r.left(),     r.top(),     6, 4);
        QRect t2(r.left() + 6, r.top() + 1, 6, 3);
        p.drawRect(t1);
        p.drawRect(t2);
        // body
        QRect body(r.left(), r.top() + 4, r.width(), r.height() - 4);
        p.drawRect(body);

    } else if (type == "PictureBox") {
        p.drawRect(r);
        // mountain triangle inside
        QPainterPath m;
        m.moveTo(r.left() + 2,         r.bottom() - 2);
        m.lineTo(r.left() + r.width() / 3, r.top() + 4);
        m.lineTo(r.center().x() + 1,   r.center().y());
        m.lineTo(r.right() - 2,        r.bottom() - 2);
        m.closeSubpath();
        p.fillPath(m, QColor(0x5b, 0x8c, 0xff));

    } else if (type == "ProgressBar") {
        p.drawRoundedRect(r, 2, 2);
        QRect fill = r.adjusted(2, 2, -r.width() / 2, -2);
        p.fillRect(fill, QColor(0x22, 0xc5, 0x5e));

    } else if (type == "Separator") {
        p.drawLine(r.left() + 1, r.center().y(), r.right() - 1, r.center().y());

    } else if (type == "Slider") {
        p.drawLine(r.left() + 2, r.center().y(), r.right() - 2, r.center().y());
        QRect knob(r.center().x() - 3, r.center().y() - 3, 6, 6);
        p.setBrush(QColor(0x5b, 0x8c, 0xff));
        p.setPen(Qt::NoPen);
        p.drawEllipse(knob);

    } else if (type == "DateTimePicker") {
        p.drawRoundedRect(r, 2, 2);
        // calendar grid hint
        p.drawLine(r.left() + 1, r.top() + 4, r.right() - 1, r.top() + 4);
        for (int x = r.left() + 4; x < r.right() - 1; x += 3)
            p.drawLine(x, r.top() + 5, x, r.bottom() - 1);

    } else if (type == "NumericUpDown") {
        QRect body(r.left(), r.top(), r.width() - 5, r.height());
        p.drawRoundedRect(body, 2, 2);
        // up/down spinner
        QRect spin(body.right() + 1, r.top(), 4, r.height());
        p.drawRect(spin);
        p.drawLine(spin.center().x(), spin.top() + 2,
                   spin.center().x(), spin.bottom() - 2);
    }

    return pm;
}
