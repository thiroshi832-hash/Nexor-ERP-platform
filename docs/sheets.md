# Sheets & Entities

A **Sheet** is a schema. Once defined, the language exposes it as a typed
object that creates / finds / queries / saves rows of that schema. Storage is
SQLite locally; via [server-side activities](server-side.md) the same calls
become REST against Core.

## 1. A Sheet on disk

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Sheet version="1">
  <Meta>
    <Title>Customer</Title>
    <Id>Customer</Id>
    <Description>Master record of trading partners.</Description>
    <Author>jdoe</Author>
    <Created>2026-04-29T10:00:00</Created>
  </Meta>
  <Fields>
    <Field name="Id"      type="Long"    key="true" required="true"/>
    <Field name="Name"    type="String"  required="true"/>
    <Field name="Email"   type="String"/>
    <Field name="Balance" type="Decimal" default="0"/>
    <Field name="Active"  type="Boolean" default="true"/>
  </Fields>
</Sheet>
```

| Field attribute | Meaning |
|---|---|
| `name`     | column name (case-insensitive on lookup) |
| `type`     | `String`, `Long`, `Integer`, `Double`, `Decimal`, `Boolean`, `Date`, `Variant` |
| `key`      | `true` / `false` — primary key (every sheet has at least an `Id`) |
| `required` | `true` / `false` — `NOT NULL` in SQL, refused on save when empty |
| `default`  | string form of the default value used when a fresh entity is created |

Studio's **Sheet Editor** is a tabular UI on top of this XML. Right-click
**Sheets → New Sheet…** to add one.

## 2. The runtime mapping

| Nexor type | SQLite column type |
|------------|--------------------|
| `Long`, `Integer`, `Boolean` | `INTEGER` |
| `Double`, `Decimal`           | `REAL` |
| `String`, `Date`, `Variant`   | `TEXT` |

Entity tables live in:

- **Studio / Flux** — per-project / per-package SQLite at `<projectRoot>/project.ndb` (Studio) or `%APPDATA%/NexorFlux/...` (Flux).
- **Core** — central SQLite at `<dataRoot>/core_entities.db`. Sheets get auto-registered when their package is published (see [Publishing](publishing.md)).

Schema migrations are **additive only** today: new columns get added on the
fly via `ALTER TABLE`. Removing or retyping a column is a change Command
flags as a "schema migration" in the deploy diff (see
[`PackageDiff`](../nexor_studio/src/build/PackageDiff.h)) — actually executing
those migrations is on the Phase 11b roadmap.

## 3. The Entity API

Every defined Sheet is reachable from any Sub by name:

```vbnet
Sub MakeOne()
    Dim c = Customer.New()           ' fresh, unsaved, Id = 0
    c.Name    = "Acme"
    c.Email   = "billing@acme.com"
    c.Balance = 1234.56
    c.Save()                          ' INSERT - now has a real Id
    Print "Saved as #" & c.Id
End Sub

Sub LookOne()
    Dim c = Customer.Find(7)
    If c Is Nothing Then
        Print "no #7"
    Else
        Print c.Name
    End If
End Sub

Sub UpdateOne()
    Dim c = Customer.Find(7)
    c.Active = False
    c.Save()                          ' UPDATE
End Sub

Sub RemoveOne()
    Customer.Delete(7)                ' single-shot remove by id
End Sub
```

| Sheet method | Returns | Notes |
|---|---|---|
| `Sheet.New()`   | a fresh `Entity`            | not yet persisted |
| `Sheet.Find(id)`| `Entity` or `Nothing`       | exact match by primary key |
| `Sheet.All()`   | a `List` of every entity    | rarely needed — prefer queries |
| `Sheet.Count()` | a `Long`                    | total rows |
| `Sheet.Delete(id)` | `Boolean`                | true on success |

| Entity method | Returns | Notes |
|---|---|---|
| `entity.Save()`   | `Boolean` | `true` on persist; `false` if validation fails |
| `entity.Delete()` | `Boolean` | removes via primary key |
| `entity.<Field>`  | the value | unknown fields read as `Empty` |
| `entity.<Field> = x` | nothing | writes through to the in-memory entity; persists on next `Save()` |

## 4. Iterating

```vbnet
For Each c In Customer
    Print c.Name & "  $" & c.Balance
Next c
```

Sheet identifiers used as the source of `For Each` auto-expand via `Sheet.All()`.

## 5. LINQ-style queries

The query language is structural rather than string-based — keywords are
parsed as language tokens:

```vbnet
Dim recent = From c In Customer _
             Where c.Balance > 1000 _
             OrderBy c.Name _
             Select c
```

Clauses (in order; only `From` is mandatory):

```
From <var> In <source>
[ Where <boolExpr> ]
[ OrderBy <expr> [Ascending|Descending] {, <expr> [Asc|Desc]} ]
[ Take <n> ]
[ Select <expr> ]
```

Examples:

```vbnet
' Top 5 by balance, descending
Dim biggest = From c In Customer _
              OrderBy c.Balance Descending _
              Take 5 _
              Select c

' Names only (projection)
Dim names = From c In Customer _
            Where c.Active _
            Select c.Name

' Cross-sheet — works as long as both are registered
Dim ordersOf = From o In Orders _
               Where o.CustomerId = currentCustomer.Id _
               OrderBy o.PlacedAt Descending _
               Select o
```

`<source>` can be:

- a Sheet identifier (`Customer`) — auto-expanded to `Customer.All()`
- a `List` value (e.g. the result of another query)
- the result of any expression that evaluates to a `List`

Today queries execute **in-memory** after pulling the source into a list.
Filter pushdown into SQL is on the Phase 5b roadmap; until then, prefer
`Find()` for single-row lookups.

## 6. Fields and types — gotchas

- A field marked `key="true"` and named anything other than `Id` is allowed,
  but the auto-allocated id always lives in the `Id` field. The first key
  field is what `Find` consults.
- `Variant` accepts anything; this is how you store free-form JSON or
  sparse data.
- `Date` fields are stored as ISO 8601 text (e.g. `2026-04-29T10:00:00`).
  The `Now()` builtin returns the right shape.

## 7. Server-side entities

When the same code runs server-side (because the activity is annotated
`[Activity(RunsOn := ServerOnly)]`), `Customer.Find(7)` resolves against
Core's `core_entities.db`, not the host's local DB. The Sheet schemas there
get registered automatically on every package deploy — so a Customer Sheet
that landed in `Sales 0.4.1` is queryable via:

```bash
curl -H "Authorization: Bearer $token" \
     http://localhost:7421/api/v1/entities/Customer
```

See [Server-side activities](server-side.md) and the
[API reference](api-reference.md) for the full HTTP shape.

## 8. Validation

`required="true"` fields without a value cause `entity.Save()` to return
`False` and write a message to the host's error sink. Beyond that, validation
today is whatever you write in your Sub:

```vbnet
Function ValidateCustomer(c) As Boolean
    If Trim(c.Name) = "" Then
        MsgBox "Name is required"
        Return False
    End If
    If c.Balance < 0 Then
        MsgBox "Balance cannot be negative"
        Return False
    End If
    Return True
End Function

Sub btnSave_Click()
    Form.Save()                            ' no - we want to validate first
End Sub
```

Better:

```vbnet
Sub btnSave_Click()
    Dim c = Form.Current
    If Not ValidateCustomer(c) Then Exit Sub
    Form.Save()
End Sub
```

## 9. Where to read next

- **[Forms Guide](forms.md)** — bind widgets to fields with the `binding=` attribute.
- **[Server-side Activities](server-side.md)** — entity calls over the network.
- **[Core API Reference](api-reference.md)** — the REST endpoints behind the entity API.
