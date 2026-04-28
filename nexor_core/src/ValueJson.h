// =============================================================================
// ValueJson — small bridge between Nexor's runtime Value and JSON, used by
// Core's RPC endpoint and by Flux's RPC bridge.
//
// Mapping:
//   nx::Value::Empty   <->  JSON null
//   nx::Value::Bool    <->  JSON true/false
//   nx::Value::Long    <->  JSON number (integer)
//   nx::Value::Double  <->  JSON number (double)
//   nx::Value::String  <->  JSON string
//   nx::Value::List    <->  JSON array
//   nx::Value::Object  <->  JSON object  ({"$kind": "...", ...} envelope -
//                          handles for kind=="Entity" surface field map;
//                          other kinds round-trip as opaque "$kind" strings.)
// =============================================================================
#ifndef NEXOR_CORE_VALUEJSON_H
#define NEXOR_CORE_VALUEJSON_H

#include <QJsonValue>
#include "../../nexor_studio/src/language/Value.h"

namespace nx {

QJsonValue valueToJson(const Value &v);
Value      jsonToValue(const QJsonValue &j);

} // namespace nx

#endif // NEXOR_CORE_VALUEJSON_H
