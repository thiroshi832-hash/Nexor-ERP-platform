#include "WidgetFactory.h"

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QListWidget>
#include <QGroupBox>
#include <QFrame>
#include <QTabWidget>
#include <QProgressBar>
#include <QSlider>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QDateTime>
#include <QColor>

// Helper: re-apply combined fg/bg stylesheet from the stored dynamic
// properties.  Called after either colour changes.
namespace {
void applyColors(QWidget *w) {
    QColor fg = w->property("nexorFg").value<QColor>();
    QColor bg = w->property("nexorBg").value<QColor>();
    QString css;
    if (fg.isValid()) css += QString("color:%1;").arg(fg.name());
    if (bg.isValid()) css += QString("background-color:%1;").arg(bg.name());
    w->setStyleSheet(css);
}
} // namespace

QWidget *WidgetFactory::create(const QString &type, QWidget *parent) {
    if (type == "Button")          return new QPushButton(QStringLiteral("Button"), parent);
    if (type == "Label")           return new QLabel(QStringLiteral("Label"), parent);
    if (type == "TextBox")         return new QLineEdit(parent);
    if (type == "TextArea")        return new QPlainTextEdit(parent);
    if (type == "CheckBox")        return new QCheckBox(QStringLiteral("CheckBox"), parent);
    if (type == "RadioButton")     return new QRadioButton(QStringLiteral("RadioButton"), parent);
    if (type == "ComboBox")        { auto *c = new QComboBox(parent);
                                     c->addItems({"Item 1", "Item 2", "Item 3"});
                                     return c; }
    if (type == "ListBox")         { auto *l = new QListWidget(parent);
                                     l->addItems({"Item 1", "Item 2", "Item 3"});
                                     return l; }
    if (type == "GroupBox")        return new QGroupBox(QStringLiteral("GroupBox"), parent);
    if (type == "Panel")           { auto *f = new QFrame(parent);
                                     f->setFrameShape(QFrame::Box);
                                     f->setFrameShadow(QFrame::Sunken);
                                     return f; }
    if (type == "TabControl")      { auto *t = new QTabWidget(parent);
                                     t->addTab(new QWidget, "Tab 1");
                                     t->addTab(new QWidget, "Tab 2");
                                     return t; }
    if (type == "PictureBox")      { auto *l = new QLabel("[Picture]", parent);
                                     l->setFrameShape(QFrame::Box);
                                     l->setAlignment(Qt::AlignCenter);
                                     return l; }
    if (type == "ProgressBar")     { auto *p = new QProgressBar(parent);
                                     p->setRange(0, 100); p->setValue(50);
                                     return p; }
    if (type == "Separator")       { auto *f = new QFrame(parent);
                                     f->setFrameShape(QFrame::HLine);
                                     f->setFrameShadow(QFrame::Sunken);
                                     return f; }
    if (type == "Slider")          { auto *s = new QSlider(Qt::Horizontal, parent);
                                     s->setRange(0, 100); s->setValue(50);
                                     return s; }
    if (type == "DateTimePicker")  return new QDateTimeEdit(QDateTime::currentDateTime(), parent);
    if (type == "NumericUpDown")   { auto *s = new QSpinBox(parent);
                                     s->setRange(0, 100); return s; }
    return nullptr;
}

bool WidgetFactory::hasTextProperty(const QString &type) {
    static const QStringList yes = {
        "Button", "Label", "TextBox", "TextArea",
        "CheckBox", "RadioButton", "GroupBox", "PictureBox"
    };
    return yes.contains(type);
}

void WidgetFactory::applyProperty(QWidget *w, const QString &key, const QVariant &v) {
    if (!w) return;
    if (key == "text") {
        if (auto *b = qobject_cast<QAbstractButton*>(w))      { b->setText(v.toString());     return; }
        if (auto *l = qobject_cast<QLabel*>(w))               { l->setText(v.toString());     return; }
        if (auto *e = qobject_cast<QLineEdit*>(w))            { e->setText(v.toString());     return; }
        if (auto *pe = qobject_cast<QPlainTextEdit*>(w))      { pe->setPlainText(v.toString()); return; }
        if (auto *gb = qobject_cast<QGroupBox*>(w))           { gb->setTitle(v.toString());   return; }
        return;
    }
    if (key == "fgColor" || key == "bgColor") {
        QColor c = v.canConvert<QColor>() ? v.value<QColor>() : QColor(v.toString());
        w->setProperty(key == "fgColor" ? "nexorFg" : "nexorBg", c);
        applyColors(w);
        return;
    }
    if (key == "visible") {
        // Stored only — at design-time we always show.  Runtime applies it.
        w->setProperty("nexorVisible", v.toBool());
        return;
    }
    if (key == "anchor") {
        w->setProperty("nexorAnchor", v.toString());
        return;
    }
}

QVariant WidgetFactory::readProperty(const QWidget *w, const QString &key) {
    if (!w) return {};
    if (key == "text") {
        if (auto *b  = qobject_cast<const QAbstractButton*>(w))  return b->text();
        if (auto *l  = qobject_cast<const QLabel*>(w))           return l->text();
        if (auto *e  = qobject_cast<const QLineEdit*>(w))        return e->text();
        if (auto *pe = qobject_cast<const QPlainTextEdit*>(w))   return pe->toPlainText();
        if (auto *gb = qobject_cast<const QGroupBox*>(w))        return gb->title();
    }
    if (key == "fgColor")  return w->property("nexorFg");
    if (key == "bgColor")  return w->property("nexorBg");
    if (key == "visible")  return w->property("nexorVisible");
    if (key == "anchor")   return w->property("nexorAnchor");
    return {};
}

QSize WidgetFactory::defaultSize(const QString &type) {
    if (type == "Button" || type == "CheckBox" || type == "RadioButton") return {100, 28};
    if (type == "Label")           return {80, 22};
    if (type == "TextBox")         return {160, 24};
    if (type == "TextArea")        return {200, 100};
    if (type == "ComboBox")        return {140, 24};
    if (type == "ListBox")         return {140, 100};
    if (type == "GroupBox")        return {200, 120};
    if (type == "Panel")           return {200, 120};
    if (type == "TabControl")      return {220, 140};
    if (type == "PictureBox")      return {120, 120};
    if (type == "ProgressBar")     return {200, 20};
    if (type == "Separator")       return {200, 4};
    if (type == "Slider")          return {160, 22};
    if (type == "DateTimePicker")  return {160, 24};
    if (type == "NumericUpDown")   return {80, 24};
    return {100, 24};
}

QString WidgetFactory::namePrefix(const QString &type) {
    if (type == "Button")          return "btn";
    if (type == "Label")           return "lbl";
    if (type == "TextBox")         return "txt";
    if (type == "TextArea")        return "txta";
    if (type == "CheckBox")        return "chk";
    if (type == "RadioButton")     return "rad";
    if (type == "ComboBox")        return "cmb";
    if (type == "ListBox")         return "lst";
    if (type == "GroupBox")        return "grp";
    if (type == "Panel")           return "pnl";
    if (type == "TabControl")      return "tab";
    if (type == "PictureBox")      return "pic";
    if (type == "ProgressBar")     return "prg";
    if (type == "Separator")       return "sep";
    if (type == "Slider")          return "sld";
    if (type == "DateTimePicker")  return "dtp";
    if (type == "NumericUpDown")   return "num";
    return "ctrl";
}
