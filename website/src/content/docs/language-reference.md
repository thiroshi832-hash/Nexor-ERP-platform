---
title: 'Language reference'
description: 'Every keyword, operator, statement, and built-in function.'
---

Nexor is a small, dynamically-typed VB-Script-flavored language. Source is
case-insensitive on keywords and identifiers; string literals preserve case.
Top-level units are `Sub` and `Function` declarations; statements outside any
sub run once at module load (this is where `Dim` for module-level globals goes).

## 1. Syntax at a glance

```vbnet
Dim greeting As String = "Hello"

[Activity(RunsOn := Either)]
Function Greet(name As String) As String
    Return greeting & ", " & name
End Function

Sub Main()
    Print Greet("world")            ' "Hello, world"
    For i = 1 To 3
        Print i & ": " & i * i
    Next i
End Sub
```

- Statements are line-terminated. `:` is the in-line statement separator.
- `'` starts a comment to end-of-line.
- `&` concatenates; `+` adds (numerics) or concatenates (when either side is a string).
- All scoping is lexical; `Sub` and `Function` bodies have their own environment chained to the module's.

## 2. Types

| Type | Literal | Notes |
|------|---------|-------|
| `Empty` (the implicit one) | (no literal) | the default value of an unassigned variable |
| `Boolean` | `True`, `False` | |
| `Long` (64-bit int) | `42`, `-1`, `0xFF` | |
| `Double` | `3.14`, `-0.5`, `1.0e6` | |
| `String` | `"text"`, `"a ""b"""` | doubled quote escapes a quote |
| `Date` | `Now()`, parsed from string | stored as ISO 8601; usually as a string field |
| `List` | result of `From … Select` queries | |
| `Object` | entities, sheets, forms, processes | tagged with a kind (`"Entity"`, `"Sheet"`, `"Form"`, `"Process"`, `"Vars"`) |

Types coerce silently in operators (e.g. `Print 1 & "x"` → `"1x"`).
`Nothing` is a synonym for `Empty` and round-trips as the zero value of any field.

## 3. Variables and scoping

```vbnet
Dim x As Integer
Dim total As Decimal = 0
Dim name = "Alice"          ' type inferred from the literal
```

`As <Type>` is documentation today — there is no static checking. The values
hold their actual type at runtime.

Module-level `Dim` runs once during `Interpreter::load`. Variables declared
inside a Sub belong to that Sub's environment.

## 4. Operators

```
Arithmetic   +  -  *  /  Mod
Concat       &
Compare      =  <>  <  <=  >  >=
Logical      And  Or  Not
Member       .                   (e.g. Customer.Name)
Call         (args)              (e.g. Greet("x"))
Index        (i)                 same shape as a call when applied to a List
Unary        -  Not
```

`=` is reused for both assignment and equality — the parser figures it out from
context (left-of-`=` is an assignment target, right-of-comparison is equality).

## 5. Statements

### `Dim`

```vbnet
Dim a              ' = Empty
Dim b As Long
Dim c = 10
Dim d As Double = 1.5
```

### `If` / `ElseIf` / `Else`

```vbnet
If amt > 1000 Then
    Print "big"
ElseIf amt > 100 Then
    Print "medium"
Else
    Print "small"
End If
```

### `While` / `Wend`

```vbnet
Dim i = 0
While i < 5
    Print i
    i = i + 1
Wend
```

### `For` / `Next`

```vbnet
For i = 1 To 10 Step 2
    Print i           ' 1 3 5 7 9
Next i
```

### `For Each` / `In` / `Next`

```vbnet
For Each c In Customer
    Print c.Name
Next c
```

When the source is a sheet (`Customer` rather than a list), the engine
auto-expands it via `Customer.All()`.

### `Return`, `Exit Sub`, `Exit Function`

```vbnet
Function FormatTotal(amt As Decimal) As String
    If amt < 0 Then Return "—"
    Return "$" & amt
End Function
```

### `Print`

```vbnet
Print "x =", x
```

`Print` joins its arguments with a single space and appends a newline. Output
flows through whatever `OutputCallback` the host installed (Studio's OUTPUT
pane, Flux's log strip, Core's RPC response envelope).

### `Call`

`Call <expr>` is optional; bare `<expr>` works equally for sub invocations.
Useful when the sub has no return value and you want the keyword for clarity.

## 6. Sub & Function declarations

```vbnet
Sub Hello(name As String)
    Print "Hello, " & name
End Sub

Function Sum(a, b)
    Return a + b
End Function
```

- `Public` / `Private` are accepted and ignored.
- Parameters' `As <Type>` annotations are documentation.
- A function without `Return` returns `Empty`.

## 7. Annotations

```vbnet
[Activity(RunsOn := ServerOnly, RequiresPermission := "Sales.Approve")]
Public Sub ApproveOrder(orderId As Long)
    ' Server-side body; clients cannot run this locally.
End Sub
```

| Key             | Values | Effect |
|-----------------|--------|--------|
| `RunsOn`        | `ServerOnly`, `ClientOnly`, `Either` (default) | `[ServerOnly]` calls from a client interpreter route through the RPC bridge transparently. `[ClientOnly]` is refused on Core. See [Server-side activities](server-side.md). |
| `RequiresPermission` | string  | reserved — Phase 9c-onwards permission gate |
| `RequiresRole`  | string  | reserved — admin-only gate |

The annotation map sits on `SubDecl` and is preserved verbatim through the
package format (it ends up in the activity's body source, parsed every time
the host compiles).

## 8. Built-in functions

### Output

| Name      | Signature                           | Notes |
|-----------|-------------------------------------|-------|
| `Print`   | `Print arg1, arg2, …`               | space-joined, newline-terminated |
| `MsgBox`  | `MsgBox(text)`                      | modal info dialog (Studio + Flux); falls back to `Print` on Core |
| `InputBox`| `InputBox(prompt, default)`         | returns the user's text; on headless hosts returns `default` |

### Conversions

```vbnet
CStr(123)        ' "123"
CInt("42")       ' 42
CLng("9000")     ' 9000
CDbl("3.14")     ' 3.14
CBool("true")    ' True
```

### Strings

```vbnet
Len("hello")          ' 5
UCase("abc")          ' "ABC"
LCase("ABC")          ' "abc"
Trim("  x  ")         ' "x"
Left("nexor", 3)      ' "nex"
Right("nexor", 3)     ' "xor"
Mid("nexor", 2, 3)    ' "exo"   (1-based)
```

### Math

```vbnet
Abs(-5)               ' 5
Int(3.9)              ' 3
Round(3.456, 2)       ' 3.46
```

### Date

```vbnet
Now()                 ' current local datetime (string, ISO 8601)
```

## 9. Member access

| Object kind  | What `obj.Foo` means | What `obj.Foo()` means |
|---|---|---|
| `Sheet` (e.g. `Customer`) | n/a | dispatches a sheet method (`New`, `Find`, `All`, `Count`, `Delete`) |
| `Entity` (e.g. `cust`) | reads field `Foo` | calls `Save` / `Delete` on the entity |
| `Form` | reads widget value or `Form.Current`, `Form.DataSource` | dispatches form action (`Save`, `Load`, `New`, `Delete`, `Accept`, `Reject`, `Close`) |
| `Vars` | reads process-scoped variable `Foo` | n/a |
| `Process` (e.g. `OrderApproval`) | n/a | `OrderApproval.Start()` runs the named process |

Assignment `obj.Foo = value` supports Entity (writes the field), Form (writes
the widget value), Vars (sets the process-scoped variable).

## 10. Sheet methods

```vbnet
Dim c = Customer.New()              ' fresh, unsaved
c.Name = "Acme"
c.Save()                             ' INSERT or UPDATE
Dim found = Customer.Find(c.Id)
Dim n     = Customer.Count()
For Each x In Customer.All()
    Print x.Name
Next x
Customer.Delete(7)
```

## 11. Queries (LINQ-style)

```vbnet
Dim recent = From c In Customer _
             Where c.Balance > 1000 _
             OrderBy c.Name _
             Select c

Dim names = From c In Customer Select c.Name
Dim top3  = From c In Customer OrderBy c.Balance Descending Take 3 Select c
```

Clauses (in order): `From <var> In <source>` (required), `Where <bool>`, `OrderBy <expr> [Ascending|Descending]` (multiple keys allowed, comma-separated), `Take <n>`, `Select <expr>` (optional — defaults to the row).

When `<source>` is a sheet identifier, the engine auto-expands via `<sheet>.All()`.

## 12. The `Form` bridge

Inside a form's `<Code>` block, the magic identifier `Form` exposes the running
dialog:

| Expression | Effect |
|---|---|
| `Form.txtName` | reads the primary value of the widget named `txtName` |
| `Form.txtName = "Hi"` | sets the widget's primary value |
| `Form.Current` | the currently bound `Entity` (when the form has a `dataSource`) |
| `Form.DataSource` | the sheet id this form is bound to |
| `Form.New()` | makes a fresh `Entity` of `DataSource`, blanks the bound widgets |
| `Form.Load(id)` | loads the entity by id and pushes its fields into bound widgets |
| `Form.Save()` | pulls bound widgets into the entity, persists |
| `Form.Delete()` | removes the current entity |
| `Form.Accept()` | closes the dialog with `accepted` (HumanTask success) |
| `Form.Reject()` | closes the dialog with `rejected` (HumanTask cancel) |
| `Form.Close()` | closes the dialog without changing the result |

## 13. The `Vars` dictionary

`Vars` is a per-process-instance dictionary. Inside a Process step body:

```vbnet
Vars.Total       = 250
Print "total=" & Vars.Total
```

Reads return `Empty` for unknown keys. The bag is JSON-serialised between
Server steps so it survives `awaiting_human` suspends on Core. See
[Processes & BPMN](processes.md).

## 14. Exceptions

There is no `Try / Catch` yet. Runtime errors (division by zero, missing sheet,
mistyped field name) produce a single message routed through the host's
`ErrorCallback` — the OUTPUT pane in Studio/Flux, the JSON `{ error: ... }`
envelope on Core's RPC endpoint.

## 15. Reserved words

```
And     As       Ascending  Boolean  By           Call         Case
CDbl    CBool    CInt       CLng     CStr         Date         Dec
Decimal Descending Dim     Do        Double       Else         ElseIf
Empty   End      Exit       False    For          From         Function
If      In       Integer    Long     Loop         Mod          Nothing
Not     Now      Or         OrderBy  Print        Private      Public
Return  Select   Set        Step     String       Sub          Take
Then    To       True       Until    Variant      Wend         Where
While
```

Identifiers are case-insensitive — `customer`, `Customer`, and `CUSTOMER` all
refer to the same sheet.
