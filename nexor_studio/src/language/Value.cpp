#include "Value.h"
#include <cmath>

namespace nx {

Value::Value() = default;

Value Value::boolean(bool b)              { Value v; v.m_kind = Bool;   v.m_i = b ? 1 : 0; return v; }
Value Value::integer(qint64 n)            { Value v; v.m_kind = Long;   v.m_i = n;          return v; }
Value Value::real   (double d)            { Value v; v.m_kind = Double; v.m_d = d;          return v; }
Value Value::text   (const QString &s)    { Value v; v.m_kind = String; v.m_s = s;          return v; }
Value Value::nothing()                    { return Value(); }

Value Value::object(std::shared_ptr<void> obj, const QString &kindTag) {
    Value v; v.m_kind = Object; v.m_obj = std::move(obj); v.m_objectKind = kindTag;
    return v;
}
Value Value::list(QVector<Value> items) {
    Value v; v.m_kind = List; v.m_list = std::move(items);
    return v;
}

bool Value::toBool() const {
    switch (m_kind) {
    case Empty:  return false;
    case Bool:   return m_i != 0;
    case Long:   return m_i != 0;
    case Double: return m_d != 0;
    case String: return !m_s.isEmpty() && m_s.compare("false", Qt::CaseInsensitive) != 0
                                       && m_s.compare("0",     Qt::CaseInsensitive) != 0;
    case Object: return m_obj != nullptr;
    case List:   return !m_list.isEmpty();
    }
    return false;
}

qint64 Value::toLong() const {
    switch (m_kind) {
    case Empty:  return 0;
    case Bool:   return m_i ? -1 : 0;        // VB convention: True=-1, False=0
    case Long:   return m_i;
    case Double: return qint64(m_d);
    case String: return m_s.toLongLong();
    }
    return 0;
}

double Value::toDouble() const {
    switch (m_kind) {
    case Empty:  return 0;
    case Bool:   return m_i ? -1.0 : 0.0;
    case Long:   return double(m_i);
    case Double: return m_d;
    case String: return m_s.toDouble();
    }
    return 0;
}

QString Value::toText() const {
    switch (m_kind) {
    case Empty:  return "";
    case Bool:   return m_i ? "True" : "False";
    case Long:   return QString::number(m_i);
    case Double: {
        // Trim trailing zeros, keep at least one decimal if it was real
        QString s = QString::number(m_d, 'g', 15);
        return s;
    }
    case String: return m_s;
    case Object: return QString("<%1>").arg(m_objectKind.isEmpty() ? "Object" : m_objectKind);
    case List:   return QString("<List of %1>").arg(m_list.size());
    }
    return "";
}

int Value::compare(const Value &a, const Value &b) {
    if (a.isNumeric() && b.isNumeric()) {
        double x = a.toDouble(), y = b.toDouble();
        if (x < y) return -1;
        if (x > y) return  1;
        return 0;
    }
    if (a.kind() == String || b.kind() == String) {
        return a.toText().compare(b.toText());
    }
    if (a.kind() == Bool && b.kind() == Bool) {
        return (a.toLong() < b.toLong()) ? -1
             : (a.toLong() > b.toLong()) ?  1 : 0;
    }
    return a.toText().compare(b.toText());
}

static bool isReal(const Value &v) {
    return v.kind() == Value::Double;
}

Value Value::add(const Value &a, const Value &b) {
    if (a.kind() == String || b.kind() == String) return text(a.toText() + b.toText());
    if (isReal(a) || isReal(b)) return real(a.toDouble() + b.toDouble());
    return integer(a.toLong() + b.toLong());
}
Value Value::sub(const Value &a, const Value &b) {
    if (isReal(a) || isReal(b)) return real(a.toDouble() - b.toDouble());
    return integer(a.toLong() - b.toLong());
}
Value Value::mul(const Value &a, const Value &b) {
    if (isReal(a) || isReal(b)) return real(a.toDouble() * b.toDouble());
    return integer(a.toLong() * b.toLong());
}
Value Value::div(const Value &a, const Value &b) {
    double y = b.toDouble();
    if (y == 0) return real(qInf());
    return real(a.toDouble() / y);
}
Value Value::mod(const Value &a, const Value &b) {
    qint64 y = b.toLong();
    if (y == 0) return integer(0);
    return integer(a.toLong() % y);
}
Value Value::concat(const Value &a, const Value &b) {
    return text(a.toText() + b.toText());
}

} // namespace nx
