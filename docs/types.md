# Nexor Type Taxonomy

Canonical list of data types Nexor exposes (or intends to expose) to user code.
This file is the source of truth — `language-reference.md` summarises a subset,
`Sheet` field schemas pull from the same vocabulary, and the SQL persistence
layer (`EntityStore`) maps these onto SQLite storage classes.

Each row records the **status** today so anyone picking up future work knows
what is wired end-to-end vs. what is aspirational.

Status legend:

- ✓ wired in `Value`, schema, parser, persistence, and runtime dispatch
- ⚠ partially wired — listed somewhere but a layer treats it as a stand-in
  (usually String or Double)
- ✗ not implemented yet — reserved name only

## 1. Primitive

Single-value scalars. No internal structure.

| Type      | Literal / construction       | Status | Notes |
|-----------|------------------------------|--------|-------|
| `Boolean` | `True`, `False`              | ✓      | `Value::Bool`, SQL `INTEGER` (0/1) |
| `Integer` | `42`, `-1`, `0xFF`           | ✓      | 64-bit; alias `Long`. `Value::Long`, SQL `INTEGER` |
| `Double`  | `3.14`, `1.0e6`              | ✓      | IEEE-754 binary64. `Value::Double`, SQL `REAL` |
| `Decimal` | `12.50d` *(planned)*          | ⚠      | Schema accepts it; runtime uses `Value::Double` and SQL `REAL`. Exact-decimal semantics still owed. |
| `String`  | `"text"`, `"a ""b"""`        | ✓      | UTF-16 internally (`QString`). SQL `TEXT` |
| `Binary`  | *(no literal yet)*           | ✗      | Will map to `BLOB`. No `Value` kind today. |

## 2. Time

Calendar / clock values. All canonical forms are ISO 8601 in storage.

| Type       | Literal / construction         | Status | Notes |
|------------|--------------------------------|--------|-------|
| `Date`     | `#2026-05-02#`, `Today()`      | ⚠      | Schema accepts `Date`; persisted as `TEXT`. No dedicated `Value` kind — round-trips as String. |
| `Time`     | `#14:30:00#`                   | ✗      | Reserved. |
| `DateTime` | `#2026-05-02T14:30:00#`, `Now()` | ✗    | Reserved. `Now()` already exists in some samples but returns a String today. |

## 3. Business

Domain shapes — types whose behaviour is meaningful to business logic
beyond their underlying storage.

| Type       | Construction                      | Status | Notes |
|------------|-----------------------------------|--------|-------|
| `Currency` | *(planned: `Decimal` + ISO code)* | ✗      | Distinct from `Decimal` because of arithmetic-with-currency-tag semantics. |
| `Enum`     | `Enum Status … End Enum`          | ✗      | Schema modelling is open. Likely a named set of String tags backed by INTEGER ordinals. |
| `Record`   | structural value type             | ✗      | An `Entity` (sheet row) is shaped like a record but is reference-typed and SQL-backed. A pure structural `Record` would be a value-typed bag of named fields. |

## 4. Data

Collection-shaped types that user code can iterate, filter, persist.

| Type    | Construction           | Status | Notes |
|---------|------------------------|--------|-------|
| `Table` | `Sheet` + `EntityStore` | ✓      | The primary persistent collection. One SQLite table per registered sheet, accessed via the sheet's name (`Customer.Find(1)` etc.). |
| `Query` | `From x In src Where … Select …` | ✓      | Optional but powerful. Parsed as `QueryExpr`; today evaluated in-memory after pulling from source. SQL push-down is a Phase 5b task. |

## 5. System

Identity, absence, and language-level singletons.

| Type   | Construction      | Status | Notes |
|--------|-------------------|--------|-------|
| `Guid` | `NewGuid()`       | ✗      | Internally `QUuid` is used for SQL connection names but the language has no `Guid` value kind or builtin yet. |
| `Null` | `Nothing`         | ✓      | `Value::Empty`. `x Is Nothing` and `x IsNot Nothing` already work. |

## 6. Integration

Interchange formats — types that exist primarily to leave / enter the
process boundary (RPC, persistence of variant data, file IO).

| Type    | Construction          | Status | Notes |
|---------|-----------------------|--------|-------|
| `Json`  | `JsonParse(s)`, `JsonStringify(v)` | ✗ | No kind, no builtins. |
| `Array` | `[1, 2, 3]` *(planned)* | ⚠ | `Value::List` exists and is produced by `Query` results. No literal syntax, no `(i)` indexing dispatch (today `lst(0)` returns the list itself). No `Array` schema field type. |

## Cross-cutting notes

- **Schema vocabulary.** `SheetSchemaField::type` is a free-form string today;
  the canonical names from this taxonomy are what tooling (forms, code
  generators, validators) should treat as well-known. Anything else falls
  back to `TEXT`.
- **SQL mapping.** Lives in `EntityTable::sqlType`
  (`nexor_studio/src/language/EntityStore.cpp`). When a new type lands, both
  `Value` and `sqlType` need updating, plus `bindFromEntity` /
  `rowToEntity` for the round-trip.
- **`Variant`.** Not a type per se — it's the escape hatch in schema fields
  to mean "anything; persist as TEXT". Equivalent to `Empty | <any other>`
  in practice.
- **`As <Type>` annotations.** Documentation only at the moment — the
  interpreter does not enforce them. Types are tracked dynamically on
  `Value`. Static checking is a future phase.
