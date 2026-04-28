#include "ValueJson.h"
#include "../../nexor_studio/src/language/EntityStore.h"

#include <QJsonObject>
#include <QJsonArray>
#include <cmath>

namespace nx {

QJsonValue valueToJson(const Value &v) {
    switch (v.kind()) {
    case Value::Empty:   return QJsonValue::Null;
    case Value::Bool:    return v.toBool();
    case Value::Long:    return static_cast<double>(v.toLong());
    case Value::Double:  return v.toDouble();
    case Value::String:  return v.toText();
    case Value::List: {
        QJsonArray a;
        for (const auto &item : v.listRef()) a.append(valueToJson(item));
        return a;
    }
    case Value::Object: {
        QJsonObject o;
        o.insert("$kind", v.objectKind());
        if (v.objectKind() == "Entity") {
            auto e = std::static_pointer_cast<Entity>(v.objectHandle());
            if (e) {
                o.insert("$id", e->id());
                o.insert("$sheet", e->sheetId());
                QJsonObject fields;
                for (const auto &name : e->fieldNames())
                    fields.insert(name, valueToJson(e->get(name)));
                o.insert("fields", fields);
            }
        }
        return o;
    }
    }
    return QJsonValue::Null;
}

Value jsonToValue(const QJsonValue &j) {
    switch (j.type()) {
    case QJsonValue::Null:     return Value();
    case QJsonValue::Bool:     return Value::boolean(j.toBool());
    case QJsonValue::Double: {
        double d = j.toDouble();
        // Integer round-trip when the JSON number has no fractional part
        // and fits in a 64-bit integer.
        if (d == std::floor(d) && std::abs(d) < 9.2e18)
            return Value::integer(static_cast<qint64>(d));
        return Value::real(d);
    }
    case QJsonValue::String:   return Value::text(j.toString());
    case QJsonValue::Array: {
        QVector<Value> xs;
        for (const auto &item : j.toArray()) xs.append(jsonToValue(item));
        return Value::list(std::move(xs));
    }
    case QJsonValue::Object: {
        // Only the {$kind, $id, $sheet, fields} envelope is structured-
        // recognised today; everything else surfaces as Empty (the RPC
        // bridge is for primitives + entities, not arbitrary graphs).
        return Value();
    }
    default:
        return Value();
    }
}

} // namespace nx
