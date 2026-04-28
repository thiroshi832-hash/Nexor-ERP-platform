# Sample: Customer CRUD

A real CRUD app over a `Customer` Sheet — define the schema, bind a form,
load and save rows, list all customers with a query.

## What's in here

```
customer-crud/
  CustomerCrud.pro                     project file
  sheets/Customer/Customer.sht         entity schema
  activities/Sales/Sales.aba           activity (Sub Main + form list)
  activities/Sales/Customer_form.frm   detail form (bound to Customer)
  activities/Sales/Customer_list.frm   list form (uses a query)
```

## Tour

### `Customer.sht` — the schema

Four fields plus the auto-`Id`:

| Field   | Type    | Notes |
|---------|---------|-------|
| Id      | Long    | primary key (every sheet has one) |
| Name    | String  | required |
| Email   | String  | optional |
| Balance | Decimal | default 0 |
| Active  | Boolean | default true |

### `Customer_form.frm` — the detail form

Bound to the `Customer` sheet via `dataSource="Customer"`. Each input
widget carries `binding="<field>"`, so `Form.Load(id)` and `Form.Save()`
push values back and forth automatically.

Buttons wire to the [Form bridge](../../forms.md):

```vbnet
Sub btnNew_Click()       Form.New()                   End Sub
Sub btnLoad_Click()      Form.Load(CLng(Form.txtId))  End Sub
Sub btnSave_Click()      Form.Save()                  End Sub
Sub btnDelete_Click()    Form.Delete()                End Sub
```

### `Customer_list.frm` — the list form

Uses a LINQ-style query against the sheet:

```vbnet
Sub Form_Load()
    Dim active = From c In Customer _
                 Where c.Active _
                 OrderBy c.Name _
                 Select c
    Form.lstCustomers.Clear()
    For Each c In active
        Form.lstCustomers.AddItem c.Name & "  $" & c.Balance
    Next c
End Sub
```

### `Sales.aba` — Sub Main

`Sub Main` is what `Run → Run Activity (Sub Main)` invokes. We use it to
seed a couple of demo rows the first time:

```vbnet
Sub Main()
    If Customer.Count() = 0 Then
        SeedDemoData()
    End If
    Print "Customer table has " & Customer.Count() & " row(s)."
End Sub

Sub SeedDemoData()
    Dim a = Customer.New()
    a.Name = "Acme":  a.Email = "billing@acme.com": a.Balance = 1234.56
    a.Save()
    Dim b = Customer.New()
    b.Name = "Globex": b.Email = "ar@globex.com":   b.Balance = 42.0
    b.Save()
End Sub
```

## Try it

1. Open `CustomerCrud.pro` in Studio.
2. **Ctrl+F5** runs `Sub Main` and seeds the demo rows.
3. Open `Customer_list.frm` → **F5** — see the two seeded customers.
4. Open `Customer_form.frm` → **F5** — type `1` in the Id box, click **Load**, then edit and **Save**.

## Concepts to take away

- Sheets are typed schemas; the SQLite table is created automatically.
- `Form.dataSource` + `binding=` give you two-way data binding for free.
- The query language is structural — clauses parse as language tokens, no string SQL.
- `Sheet.New / .Find / .All / .Count / .Delete` covers basic CRUD without writing entity code.
- Validation (required fields, type coercion) happens at `entity.Save()`.

When you're ready, move on to [`order-approval/`](../order-approval/) which
adds a BPMN process with a Choice gateway and a HumanTask.
