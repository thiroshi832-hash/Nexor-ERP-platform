---
title: 'Sample: Hello World'
description: 'One Activity, one Form, one Button.'
---

The smallest possible Nexor project: one Activity, one Form, one Button. A
click pops a MessageBox.

## What's in here

```
hello-world/
  HelloWorld.pro                       project file
  activities/Hi/Hi.aba                 the activity (Sub Main + form list)
  activities/Hi/Hi_main.frm            the auto-generated main form
```

## Tour

**`Hi.aba`** — the activity declaration plus a tiny `Sub Main` you can run
via **Run → Run Activity (Sub Main)** (`Ctrl+F5`):

```vbnet
Sub Main()
    Print "Hello from Sub Main"
End Sub
```

**`Hi_main.frm`** — a 320×160 dialog with a label and a button. The
`<Code>` block hosts the click handler:

```vbnet
Sub btnHello_Click()
    MsgBox "Hello, " & Form.txtName & "!"
End Sub
```

The `txtName` widget is a plain `QLineEdit` with `binding=""` (no entity
backing). `Form.txtName` reads its current text — that's the whole magic
of the [Form bridge](../../forms.md).

## Try it

1. Open `HelloWorld.pro` in Studio.
2. Double-click `Hi_main` in the project tree.
3. **F5** — type a name into the textbox, click **Hello**, see the box.

## Concepts to take away

- Forms ship their event-handler code inline in `<Code>`.
- Sub names follow the convention `<widget>_<event>` (e.g. `btnHello_Click`).
- `Form.<widget>` reads the widget's primary value (text for QLineEdit).
- `MsgBox` is a stdlib builtin — no imports.

When you're ready, move on to [`customer-crud/`](../customer-crud/) which
adds a Sheet and persistent storage.
