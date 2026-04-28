# Forms Guide

A Nexor **form** is an XML file (`.frm`) describing a window, its widgets, and
the event-driven code that runs when a user interacts with it. Studio's form
designer is a WYSIWYG canvas; the same XML drives the form at runtime in Flux.

## 1. Anatomy of a `.frm`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Form version="1" dataSource="Customer">
  <Title>Customer Detail</Title>
  <Geometry width="480" height="280"/>

  <Widget type="QLabel"     name="lblName" x="20" y="20" width="80"  height="24">
    <Property name="text">Name:</Property>
  </Widget>
  <Widget type="QLineEdit"  name="txtName" x="120" y="20" width="320" height="24"
                            binding="Name"/>

  <Widget type="QPushButton" name="btnSave" x="200" y="220" width="100" height="30">
    <Property name="text">Save</Property>
  </Widget>

  <Code><![CDATA[
    Sub Form_Load()
        Form.New()
    End Sub

    Sub btnSave_Click()
        Form.Save()
        MsgBox "Saved"
        Form.Close()
    End Sub
  ]]></Code>
</Form>
```

Key concepts:

- **`dataSource`** (Form attribute) names the Sheet this form is bound to. Optional.
- **`binding`** (Widget attribute) names the field whose value should populate / receive this widget.
- **`<Code>`** carries Nexor source. The runtime auto-wires every `<widget>_<event>` sub it finds (e.g. `btnSave_Click`).

## 2. Widget palette

| Widget type        | Primary value | Maps to Qt class | Default event |
|--------------------|---------------|------------------|---------------|
| `QPushButton`      | text          | QPushButton      | `Click`       |
| `QLabel`           | text          | QLabel           | (none)        |
| `QLineEdit`        | text          | QLineEdit        | `Change`      |
| `QPlainTextEdit`   | plain text    | QPlainTextEdit   | `Change`      |
| `QCheckBox`        | checked (Bool)| QCheckBox        | `Change`      |
| `QRadioButton`     | checked (Bool)| QRadioButton     | `Change`      |
| `QComboBox`        | currentText   | QComboBox        | `Change`      |
| `QSpinBox`         | value (Long)  | QSpinBox         | `Change`      |
| `QDoubleSpinBox`   | value (Double)| QDoubleSpinBox   | `Change`      |
| `QSlider`          | value (Long)  | QSlider          | `Change`      |
| `QDateTimeEdit`    | dateTime (ISO)| QDateTimeEdit    | `Change`      |
| `QProgressBar`     | value (Long)  | QProgressBar     | (none)        |

Widgets carry XML attributes for geometry (`x`, `y`, `width`, `height`),
`name` (used as the identifier in code), `binding` (entity field), and inner
`<Property name="…">value</Property>` for Qt properties not represented above.

## 3. Standard events

The form runner wires Qt signals → Nexor sub calls automatically:

| Sub name pattern    | Fires when |
|---------------------|------------|
| `Form_Load`         | once, right after the dialog is shown |
| `Form_Unload`       | once, when the dialog is destroyed |
| `<name>_Click`      | the widget is clicked (or `Enter` is pressed inside a `QLineEdit`) |
| `<name>_Change`     | the widget's value changes (every keystroke / toggle / spin) |

Subs you don't write are silently skipped — the runtime only calls what's
defined.

## 4. The `Form` bridge

Inside a form's `<Code>`, the special identifier **`Form`** exposes both the
widgets (by name) and a small set of methods that drive the bound Sheet:

```vbnet
Sub btnNew_Click()
    Form.New()                          ' new entity, widgets blanked
End Sub

Sub btnLoad_Click()
    Form.Load(CLng(Form.txtId))         ' fetch by id
End Sub

Sub btnSave_Click()
    If Form.Save() Then
        MsgBox "Saved as id " & Form.Current.Id
    End If
End Sub

Sub btnDelete_Click()
    If Form.Delete() Then Form.New()
End Sub
```

Reads (`Form.<widget>`) return the widget's primary value (see table above).
Writes (`Form.<widget> = value`) set it. The `Form.Current` accessor returns
the bound entity (or `Empty` when no `dataSource`).

For HumanTask flows (see [Processes & BPMN](processes.md)) these three matter:

```vbnet
Form.Accept()    ' closes with QDialog::Accepted - resume()s the workflow
Form.Reject()    ' closes with QDialog::Rejected - aborts the workflow
Form.Close()     ' closes without changing the result
```

## 5. Two-way data binding

When a form has both `dataSource="<sheet>"` and per-widget `binding="<field>"`
attributes:

1. `Form.Load(id)` → engine fetches the entity, copies each field into its
   bound widget via `WidgetValue::write`.
2. `Form.Save()` → engine reads each bound widget via `WidgetValue::read`,
   writes the value into the entity, then `EntityTable::save`.

Type coercion is automatic — a `QLineEdit` bound to a `Long` field round-trips
as a string in the UI and a 64-bit integer in storage. See
[`runtime/WidgetValue.h`](../nexor_studio/src/runtime/WidgetValue.h) for the
exact mapping.

## 6. Tab order

Studio's **View → Tab Order…** dialog reorders widgets — the canonical order
is the sequence they appear in the `.frm` XML.

## 7. Designing in Studio

```
┌────────────────────────────────────────────────────────────┐
│ DESIGN  ← FancyTabBar mode                                 │
├──────────────┬─────────────────────────┬──────────────────┤
│ widget       │     form canvas         │ properties       │
│ palette      │   (drag widgets here)   │  + bindings      │
│              │                         │                  │
│ • Button     │   ┌──────────────┐      │ Name:  txtName   │
│ • TextBox    │   │  txtName     │      │ Text:  …         │
│ • Label      │   └──────────────┘      │ Binding: Name    │
│ • CheckBox   │                         │                  │
│ • …          │   ┌──────────────┐      │  + generate      │
│              │   │   btnSave    │      │    btnSave_Click │
└──────────────┴───┴──────────────┴──────┴──────────────────┘
```

- **Drag** from the palette onto the canvas.
- **Click** a widget to select it; properties appear on the right.
- **Double-click** a widget to insert (or jump to) its default event handler stub.
- **F5** runs the form locally for quick smoke-testing.

## 8. Running a form from Flux

Flux's [`ActivityPicker`](../nexor_flux/src/ActivityPicker.h) tree lists every
form in the installed package. Selecting one and clicking **▶ Run Form** opens
it via the same `FormRunner` Studio uses.

The form runs against the same `EntityStore` — but Flux's runtime opens its
local SQLite under `%APPDATA%/NexorFlux/extracted/<id>/<ver>/`. To talk to
Core's central database instead, use the [server-side activity model](server-side.md).

## 9. Keyboard shortcuts (Studio)

| Shortcut | Action |
|---|---|
| `F5`         | Run the open form |
| `Ctrl+S`     | Save form + activity (combined) |
| `Ctrl+B`     | Build the project package |
| `F2`         | Insert default event handler for the selected widget |
| `Alt+0`      | Toggle Project panel |
| `Alt+9`      | Toggle Output pane |

## 10. Where to read next

- **[Sheets & Entities](sheets.md)** — define the `dataSource` your forms bind to.
- **[Processes & BPMN](processes.md)** — call forms as HumanTask steps in a workflow.
- **[Server-side Activities](server-side.md)** — make `Form.Save()` go to Core instead of local SQLite.
