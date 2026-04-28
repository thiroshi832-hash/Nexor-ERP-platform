#include "WidgetValue.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QAbstractButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QDateTimeEdit>
#include <QProgressBar>
#include <QDateTime>

namespace nx {

Value WidgetValue::read(const QWidget *w) {
    if (!w) return Value();
    if (auto *cb = qobject_cast<const QCheckBox*>(w))      return Value::boolean(cb->isChecked());
    if (auto *rb = qobject_cast<const QRadioButton*>(w))   return Value::boolean(rb->isChecked());
    if (auto *e  = qobject_cast<const QLineEdit*>(w))      return Value::text(e->text());
    if (auto *pe = qobject_cast<const QPlainTextEdit*>(w)) return Value::text(pe->toPlainText());
    if (auto *l  = qobject_cast<const QLabel*>(w))         return Value::text(l->text());
    if (auto *c  = qobject_cast<const QComboBox*>(w))      return Value::text(c->currentText());
    if (auto *s  = qobject_cast<const QSpinBox*>(w))       return Value::integer(s->value());
    if (auto *s  = qobject_cast<const QDoubleSpinBox*>(w)) return Value::real(s->value());
    if (auto *s  = qobject_cast<const QSlider*>(w))        return Value::integer(s->value());
    if (auto *p  = qobject_cast<const QProgressBar*>(w))   return Value::integer(p->value());
    if (auto *d  = qobject_cast<const QDateTimeEdit*>(w))
        return Value::text(d->dateTime().toString(Qt::ISODate));
    if (auto *b  = qobject_cast<const QAbstractButton*>(w)) return Value::text(b->text());
    return Value();
}

void WidgetValue::write(QWidget *w, const Value &v) {
    if (!w) return;
    if (auto *cb = qobject_cast<QCheckBox*>(w))      { cb->setChecked(v.toBool()); return; }
    if (auto *rb = qobject_cast<QRadioButton*>(w))   { rb->setChecked(v.toBool()); return; }
    if (auto *e  = qobject_cast<QLineEdit*>(w))      { e->setText(v.toText()); return; }
    if (auto *pe = qobject_cast<QPlainTextEdit*>(w)) { pe->setPlainText(v.toText()); return; }
    if (auto *l  = qobject_cast<QLabel*>(w))         { l->setText(v.toText()); return; }
    if (auto *c  = qobject_cast<QComboBox*>(w))      {
        int idx = c->findText(v.toText());
        if (idx >= 0) c->setCurrentIndex(idx);
        else          c->setEditText(v.toText());
        return;
    }
    if (auto *s  = qobject_cast<QSpinBox*>(w))       { s->setValue(int(v.toLong())); return; }
    if (auto *s  = qobject_cast<QDoubleSpinBox*>(w)) { s->setValue(v.toDouble()); return; }
    if (auto *s  = qobject_cast<QSlider*>(w))        { s->setValue(int(v.toLong())); return; }
    if (auto *p  = qobject_cast<QProgressBar*>(w))   { p->setValue(int(v.toLong())); return; }
    if (auto *d  = qobject_cast<QDateTimeEdit*>(w))  {
        QDateTime dt = QDateTime::fromString(v.toText(), Qt::ISODate);
        if (dt.isValid()) d->setDateTime(dt);
        return;
    }
    if (auto *b  = qobject_cast<QAbstractButton*>(w)){ b->setText(v.toText()); return; }
}

} // namespace nx
