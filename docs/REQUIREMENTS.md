# Nexor — Software Requirements Specification

**Version:** 1.0
**Status:** Draft
**Last updated:** 2026-04-30

> Conventions
> - **MUST / SHALL** = mandatory. **SHOULD** = strongly preferred. **MAY** = optional.
> - Numbered IDs (FR-xxxx, NFR-xxxx) are stable handles for cross-reference and traceability.

---

## 1 · Introduction

### 1.1 Purpose
Nexor is a self-contained ERP platform that lets a small business build, publish, and run line-of-business apps end-to-end in one toolchain — without writing JavaScript, configuring a web framework, or managing a database schema by hand. This document specifies what each of Nexor's four apps and its shared language MUST do for the platform to be considered functionally complete for its 1.0 release.

### 1.2 Scope
The platform covers four layers:

| App           | Role                                                              | UI       |
|---------------|-------------------------------------------------------------------|----------|
| **Studio**    | IDE — author Activities, design Forms, define Sheets, model Processes | Qt 5 GUI |
| **Core**      | Backend server — package registry, entity DB, RPC, workflow persistence | Console (HTTP) |
| **Command**   | Admin console — review packages, deploy, roll back, audit         | Qt 5 GUI |
| **Flux**      | End-user runtime — install and run published apps                 | Qt 5 GUI |

All four apps share a single source tree under `nexor_studio/src/`. Core, Command, and Flux pull in the parts they need (language interpreter, runtime, package format) via qmake `INCLUDEPATH`.

### 1.3 Out of scope (1.0)
- Web/mobile clients for end-users (Flux is desktop-only).
- Multi-tenant Core (one tenant per Core instance).
- Visual debugger (breakpoints, step-into, watch). Debug menu items are stubs.
- Source control inside Studio (no built-in Git/SVN client).
- Localisation / RTL.
- PostgreSQL backend for Core (SQLite only; Postgres is on the 2.0 roadmap).

### 1.4 Definitions
| Term       | Meaning                                                                  |
|------------|--------------------------------------------------------------------------|
| **Project** | The top-level container in Studio. One `.nxproj` XML file + a tree of `.aba`, `.frm`, `.sht`, `.prc` files. |
| **Activity** | A unit of executable behaviour. Two flavours: **Atomic** (`.aba` — code + forms) and **Process** (`.prc` — BPMN 2.0). |
| **Sheet** | An entity schema (`.sht`) — fields, types, key, defaults. Becomes a SQL table at runtime. |
| **Form** | A UI definition (`.frm`) — widgets, layout, event handlers in Nexor script. |
| **Process** | A BPMN 2.0 workflow with Server / Choice / HumanTask / Final steps. |
| **Package (`.nexor`)** | A single signed XML document bundling every Activity, Form, Sheet, and Process in a project, plus metadata + SHA-256 hashes. |
| **Entity** | A row of a Sheet at runtime. Has `.Save()`, `.Delete()`, typed field accessors. |
| **Vars** | A per-process shared dictionary (`Vars.X = 5`) persisted between BPMN steps. |

### 1.5 Audience
Engineers extending Studio's IDE features, language maintainers, Core API contributors, sample/docs authors, and QA writing acceptance tests.

---

## 2 · Overall Description

### 2.1 Product perspective
Nexor is a **vertical slice**: every layer from authoring to running is in one repo with one common language. The author writes VB-Script-flavored code in Studio; a `.nexor` package is built; Core receives it; Command approves it; Flux runs it. No external compiler, database server, web framework, or runtime is required for the happy path.

### 2.2 Develop → publish → run loop

```
   ┌──────────┐        ┌──────────┐        ┌──────────┐        ┌──────────┐
   │  Studio  │  build │   Core   │ deploy │  Command │  fetch │   Flux   │
   │ author   │ ─────▶ │ pending  │ ─────▶ │ approves │ ─────▶ │ runs the │
   │  + Build │        │ live     │        │  +       │        │ activity │
   │          │        │ rolled   │        │ rollback │        │          │
   └──────────┘        └──────────┘        └──────────┘        └──────────┘
```

### 2.3 User classes
| Class         | App(s) used    | Skill assumption                              |
|---------------|----------------|-----------------------------------------------|
| **Developer** | Studio         | Comfortable with VB / VBScript / Pascal-style syntax. |
| **Admin**     | Command        | Operations; reads logs, approves/rolls back packages. |
| **End user**  | Flux           | Office worker; clicks buttons in forms.       |
| **Integrator**| Core (HTTP)    | Calls REST endpoints from external systems.    |

### 2.4 Operating environments
- **Build / dev**: Qt 5.14.0 with mingw730_64 toolchain, qmake.
- **Runtime**: Windows 10/11 x64 for the Qt apps; Core runs on any platform Qt builds for.
- **Storage**: SQLite for project entity store (`project.ndb`) and Core's package store.

### 2.5 Assumptions & dependencies
- Qt 5.14.x is available with `QtCore`, `QtGui`, `QtWidgets`, `QtXml`, `QtSql`, and the SQLite driver.
- The host filesystem supports paths up to 260 chars and is case-insensitive (Windows).
- Network between Flux/Command and Core is reliable HTTP/1.1; HTTPS is the operator's responsibility (reverse proxy).

---

## 3 · Functional Requirements — Studio

### 3.1 Welcome page
| ID | Requirement |
|---|---|
| FR-STD-0010 | On launch with no project, Studio MUST display a Welcome page with "Open Project", "Open Sample", "New Project", and a recent-projects list. |
| FR-STD-0011 | Selecting a recent project MUST open it without requiring a file picker. |
| FR-STD-0012 | "Open Sample" MUST list every project under `docs/samples/` and open the chosen one in-place. |

### 3.2 Project lifecycle
| ID | Requirement |
|---|---|
| FR-STD-0100 | Studio MUST create a new project from `File → New Project…` given a name, root directory, and (optional) author. |
| FR-STD-0101 | Creating a project MUST write `<root>/<id>.nxproj` plus empty `activities/`, `sheets/`, and `processes/` sub-directories. |
| FR-STD-0102 | Opening an existing `.nxproj` MUST populate the Project tree with all referenced Activities, Sheets, and Processes. |
| FR-STD-0103 | The project tree's group nodes (Activities / Sheets / Processes) MUST offer right-click context menus for "New …" and "Insert existing …". |
| FR-STD-0104 | `File → Save` MUST persist all open editors and re-serialise the `.nxproj` if the project structure changed. |

### 3.3 Activity editor (Atomic, `.aba`)
| ID | Requirement |
|---|---|
| FR-STD-0200 | The Activity editor MUST be a code editor with VB-flavored syntax highlighting (keywords, strings, comments, numbers). |
| FR-STD-0201 | Saving a `.aba` MUST update the `<Code><![CDATA[…]]></Code>` block while preserving meta, form list, and annotations. |
| FR-STD-0202 | `Run → Run Activity (Sub Main)` (`Ctrl+F5`) MUST compile + execute `Sub Main()` in the currently open Activity, with stdout/`Print` captured into the bottom output dock. |
| FR-STD-0203 | Compile errors MUST surface in the output dock prefixed with `ERROR:` and include the file name and line number. |
| FR-STD-0204 | Runtime errors MUST surface in the output dock; the script MUST NOT crash the app. |

### 3.4 Form designer (`.frm`)
| ID | Requirement |
|---|---|
| FR-STD-0300 | The Form designer MUST present a toolbox of widgets (Label, Button, TextEdit, PlainTextEdit, CheckBox, ComboBox, ListWidget, TableWidget, SpinBox, DoubleSpinBox, GroupBox, TabWidget, Frame). |
| FR-STD-0301 | Dragging a widget from the toolbox MUST add it to the form at the drop point and assign a unique `name`. |
| FR-STD-0302 | A property panel MUST allow editing geometry, name, text, enabled, visible, and per-widget specific properties. |
| FR-STD-0303 | The XML on disk MUST round-trip — saving and re-loading a form MUST yield an identical widget set with identical properties. |
| FR-STD-0304 | The designer MUST accept Qt class names (`QPushButton`, `QLabel`, …) as widget `type` attributes in addition to friendly names (`Button`, `Label`). |
| FR-STD-0305 | An event handler editor MUST let the author attach a `Sub <name>_Click()` / `_Changed()` / `_DoubleClick()` sub to a widget. The sub lives in the parent Activity. |

### 3.5 Sheet editor (`.sht`)
| ID | Requirement |
|---|---|
| FR-STD-0400 | Right-clicking the "Sheets" group → "New Sheet…" MUST open a dialog for Title, Id, Description, Author, then write `sheets/<Id>/<Id>.sht`. |
| FR-STD-0401 | The Sheet editor MUST show a table of fields with columns: name, type (String / Long / Decimal / Double / Date / Boolean), key (exactly one), required, default. |
| FR-STD-0402 | Adding, removing, or re-typing a field and saving MUST update the `.sht` XML. |
| FR-STD-0403 | The first column flagged `key` MUST become the SQL primary key. If none is flagged, the runtime MUST synthesise an `Id INTEGER PRIMARY KEY AUTOINCREMENT`. |

### 3.6 Process designer (`.prc`, BPMN 2.0)
| ID | Requirement |
|---|---|
| FR-STD-0500 | The Process designer MUST be a canvas hosting BPMN nodes: Start, Server, Choice (gateway), HumanTask, Final. |
| FR-STD-0501 | Nodes MUST be connectable by directed sequence flows. A Choice gateway MAY have multiple outgoing flows, each guarded by a Nexor expression. |
| FR-STD-0502 | A Server step's body MUST be a Nexor script that reads/writes `Vars.<name>`. |
| FR-STD-0503 | Saving a `.prc` MUST emit valid BPMN 2.0 XML (`OMG formal/2011-01-03`) with Nexor-namespaced extensions for code bodies. |

### 3.7 Build / package
| ID | Requirement |
|---|---|
| FR-STD-0600 | `Build → Build Package` MUST produce a single `.nexor` file under `<project>/dist/`. |
| FR-STD-0601 | Every embedded text file (`.aba`, `.frm`, `.sht`, `.prc`) MUST be **line-ending-normalised to LF** before being hashed and embedded. (Prevents CRLF-vs-LF hash mismatch on read.) |
| FR-STD-0602 | Each embedded file MUST carry its SHA-256 hash; the whole `.nexor` document MUST carry an HMAC-SHA256 signature using the project's key. |
| FR-STD-0603 | The build output dock MUST show `Wrote <path> (<n> activities, <m> forms, <p> sheets, <q> processes)`. |
| FR-STD-0604 | `Build → Clean Project` MAY be a stub in 1.0 (currently disabled). |

### 3.8 Run integration
| ID | Requirement |
|---|---|
| FR-STD-0700 | `Run → Run Activity (Sub Main)` MUST open the project's SQLite store (`<root>/project.ndb`) before executing, register every Sheet, and route Sheet calls (`Customer.New()`, …) through it. |
| FR-STD-0701 | If no project is loaded (e.g. user opened a loose `.aba`), Studio MUST emit a visible error in the output dock explaining that entity persistence is disabled — not silently return 0 from `Count()`. *(Tracked: fix/entitystore-error-surface.)* |
| FR-STD-0702 | `MsgBox` MUST display a modal Qt dialog when widgets are available; fall back to text output otherwise. |
| FR-STD-0703 | `Print` MUST append a line to the output dock with a 5-digit timestamp prefix. |

### 3.9 Menus & shortcuts
| ID | Requirement |
|---|---|
| FR-STD-0800 | The main menu MUST include File, Edit, View, Build, Run, Debug, Help. |
| FR-STD-0801 | Edit → Undo/Redo, Build → Clean Project, Debug → Start Debugging, Debug → Toggle Breakpoint MAY remain disabled in 1.0 (acknowledged stubs). |
| FR-STD-0802 | Standard shortcuts: `Ctrl+N` (new project), `Ctrl+O` (open), `Ctrl+S` (save), `Ctrl+F5` (run), `Ctrl+Shift+B` (build), `F1` (help). |

---

## 4 · Functional Requirements — Core

### 4.1 Package registry
| ID | Requirement |
|---|---|
| FR-CORE-0100 | Core MUST accept `POST /packages/register` with a `.nexor` body, verify its HMAC signature, recompute every embedded SHA-256, and store on success. |
| FR-CORE-0101 | If any per-file hash or the package signature fails to verify, Core MUST reject with `400` and an error message naming the offending file. |
| FR-CORE-0102 | A registered package MUST start in status `pending`. |
| FR-CORE-0103 | `POST /packages/<id>/deploy` MUST transition status `pending → live`. The previous `live` package (if any) MUST transition to `rolled`. |
| FR-CORE-0104 | `POST /packages/<id>/rollback` MUST transition the current `live` package to `rolled` and restore the most recent prior `rolled` package to `live`. |
| FR-CORE-0105 | `GET /packages` MUST list packages with id, version, title, status, registered-at, deployed-at. |
| FR-CORE-0106 | `GET /packages/<id>` MUST return package metadata. `GET /packages/<id>/raw` MUST stream the original `.nexor`. |

### 4.2 Entity REST
| ID | Requirement |
|---|---|
| FR-CORE-0200 | For each Sheet in the live package, Core MUST expose: `GET /entities/<sheet>`, `GET /entities/<sheet>/<id>`, `POST /entities/<sheet>`, `PUT /entities/<sheet>/<id>`, `DELETE /entities/<sheet>/<id>`. |
| FR-CORE-0201 | Request/response bodies MUST be JSON with field names matching the Sheet schema (case-preserved). |
| FR-CORE-0202 | Type coercion MUST follow the Sheet field's declared type (Long, Decimal, Boolean, String, Date). |
| FR-CORE-0203 | Server-side Activities (`[Activity(RunsOn := ServerOnly)]`) MUST be invokable via `POST /rpc/<activity>/<sub>` with JSON arguments; the response MUST be the JSON-encoded return value. |

### 4.3 Process / workflow persistence
| ID | Requirement |
|---|---|
| FR-CORE-0300 | A BPMN process MUST be startable via `POST /processes/<id>/start` with initial `Vars` as JSON; Core MUST return a workflow-instance id. |
| FR-CORE-0301 | Server steps MUST run synchronously on Core during `start` or `signal`. |
| FR-CORE-0302 | When a HumanTask step is reached, Core MUST persist the instance and respond with the pending task's id and assignee. |
| FR-CORE-0303 | `POST /tasks/<id>/complete` with JSON body MUST resume the instance with the user's input merged into `Vars`. |
| FR-CORE-0304 | A Final step MUST mark the instance `done` and free its row in the workflow table. |

### 4.4 Audit & logging
| ID | Requirement |
|---|---|
| FR-CORE-0400 | Every state change (register, deploy, rollback, task complete) MUST be appended to an immutable audit log table with timestamp, actor, action, target. |
| FR-CORE-0401 | `GET /audit` MUST stream the log with optional `since=` filter. |

---

## 5 · Functional Requirements — Command

| ID | Requirement |
|---|---|
| FR-CMD-0100 | Command MUST authenticate to a Core endpoint (URL + token) and display its package list with status badges. |
| FR-CMD-0101 | Selecting a pending package MUST show a diff vs. the current `live` package (added/changed/removed activities, forms, sheets, processes). |
| FR-CMD-0102 | "Deploy" MUST send `POST /packages/<id>/deploy` and refresh the list. |
| FR-CMD-0103 | "Rollback" MUST send `POST /packages/<id>/rollback` and refresh the list. |
| FR-CMD-0104 | Command MUST display the audit log with filters by date, actor, and action. |

---

## 6 · Functional Requirements — Flux

| ID | Requirement |
|---|---|
| FR-FLX-0100 | Flux MUST connect to a Core endpoint and download the current `live` package on launch. |
| FR-FLX-0101 | Flux MUST render the package's Activities as a launcher (list / tile view). |
| FR-FLX-0102 | Launching an Activity MUST open its first Form in a window and execute `Form_Load` if defined. |
| FR-FLX-0103 | Entity calls (`Customer.New`, `.Find`, `.Save`, `.Delete`) within Flux MUST be transparently routed to Core's `/entities/` endpoints — Flux MUST NOT keep a local SQLite copy. |
| FR-FLX-0104 | Server-side activities MUST be invoked transparently via `/rpc/`; the script author writes the same code as if it were local. |
| FR-FLX-0105 | Flux MUST poll for new live packages every N seconds (configurable; default 60) and prompt the user to reload when a new version is available. |

---

## 7 · The Nexor Language

### 7.1 Lexical
| ID | Requirement |
|---|---|
| FR-LANG-0100 | Identifiers MUST be Latin alphanumerics + underscore, starting with a letter. Case-insensitive for keywords and identifier lookup. |
| FR-LANG-0101 | Numeric literals: decimal integers (`42`), decimals (`3.14`), negative via unary `-`. |
| FR-LANG-0102 | String literals: double-quoted, with `""` as the escape for a literal quote. |
| FR-LANG-0103 | Boolean literals: `True`, `False`. |
| FR-LANG-0104 | Null literal: `Nothing`. |
| FR-LANG-0105 | Comments: `'` to end of line. |
| FR-LANG-0106 | Statements MUST be newline-terminated; `:` MAY separate multiple statements on one line. |

### 7.2 Operators
| ID | Requirement |
|---|---|
| FR-LANG-0200 | Arithmetic: `+ - * / Mod` with usual precedence (`*/Mod` over `+ -`). |
| FR-LANG-0201 | Concatenation: `&` (string-favoring). |
| FR-LANG-0202 | Comparison: `= <> < <= > >=` returning Boolean. |
| FR-LANG-0203 | Logical: `And`, `Or`, `Not`. |
| FR-LANG-0204 | Identity: `Is`, `IsNot` against `Nothing`. |
| FR-LANG-0205 | Assignment: `=` (statement form), distinct from comparison by context. |
| FR-LANG-0206 | Division by zero MUST throw a runtime error, not silently return infinity. |

### 7.3 Statements
| ID | Requirement |
|---|---|
| FR-LANG-0300 | `Dim <name> [= <expr>]` declares a local. Without `=`, the variable is `Nothing`. |
| FR-LANG-0301 | `If <cond> Then … [ElseIf … Then …] [Else …] End If` — multi-line form. |
| FR-LANG-0302 | `If <cond> Then <stmt> [Else <stmt>]` — single-line form. |
| FR-LANG-0303 | `While <cond> … Wend`. |
| FR-LANG-0304 | `For i = <a> To <b> [Step <s>] … Next i`. |
| FR-LANG-0305 | `For Each <var> In <collection> … Next <var>`. |
| FR-LANG-0306 | `Sub <name>(<args>) … End Sub` and `Function <name>(<args>) [As <type>] … End Function`. |
| FR-LANG-0307 | `Return <expr>` exits a function with a value. |
| FR-LANG-0308 | `Exit Sub` / `Exit Function` / `Exit For` / `Exit While`. |
| FR-LANG-0309 | `Call <Ident>(args)` and bare `<Ident> [args]` (no parens) MUST both be valid statement-form calls. |

### 7.4 Annotations
| ID | Requirement |
|---|---|
| FR-LANG-0400 | A `[Activity(RunsOn := <Either / ClientOnly / ServerOnly>)]` annotation above a `Sub` or `Function` MUST declare where it may execute. Default: `Either`. |
| FR-LANG-0401 | Unknown annotations MUST be ignored, not error. |

### 7.5 Built-ins
| ID | Requirement |
|---|---|
| FR-LANG-0500 | String: `Len, UCase, LCase, Trim, Left, Right, Mid, InStr, Replace, Split, Join`. |
| FR-LANG-0501 | Math: `Abs, Int, Round(n, digits), Sqr, Sin, Cos, Tan, Min, Max, Rnd`. |
| FR-LANG-0502 | Conversion: `CStr, CInt, CDbl, CBool, CDate`. |
| FR-LANG-0503 | I/O: `Print, MsgBox`. `MsgBox` MUST accept any number of args; only the first is shown in the dialog body. |
| FR-LANG-0504 | `Round(n, digits)` MUST honour the digits argument (e.g. `Round(3.456, 2) = 3.46`). |

### 7.6 Object model
| ID | Requirement |
|---|---|
| FR-LANG-0600 | `Form.<control>.<property>` MUST read/write the named property on the named widget (when a Form is bound). |
| FR-LANG-0601 | `<Sheet>.New() / .Find(id) / .All() / .Count() / .Delete(id)` MUST behave as the Sheet API. |
| FR-LANG-0602 | `<entity>.<field>` MUST read/write the field; `<entity>.Save()` and `<entity>.Delete()` MUST persist or remove. |
| FR-LANG-0603 | `Vars.<name>` MUST read/write the per-process dictionary. Writes outside a process context MUST silently no-op. |
| FR-LANG-0604 | `<Process>.Start(initialVars)` MUST kick off a registered Process activity. |

---

## 8 · Data Model

### 8.1 Sheets
| ID | Requirement |
|---|---|
| FR-DAT-0100 | A Sheet MUST be persisted as `<root>/sheets/<Id>/<Id>.sht` XML with `<Meta>` and `<Fields>`. |
| FR-DAT-0101 | Each `<Field>` MUST carry `name`, `type`, optional `key`, `required`, `default`. |
| FR-DAT-0102 | Field types MUST be one of: `String, Long, Integer, Decimal, Double, Date, Boolean, Variant`. |
| FR-DAT-0103 | At runtime each Sheet maps to a SQLite table with the same name; field types map as: Long/Integer/Boolean → INTEGER, Double/Decimal → REAL, everything else → TEXT. |
| FR-DAT-0104 | The key column MUST be `INTEGER PRIMARY KEY AUTOINCREMENT`. If no field is flagged `key`, an implicit `Id` column MUST be created. |
| FR-DAT-0105 | Schema migration on register MUST be: `CREATE TABLE IF NOT EXISTS`, plus `ALTER TABLE ADD COLUMN` for new fields. The runtime MUST NOT drop or retype columns in 1.0. |

### 8.2 Entity store (`project.ndb`)
| ID | Requirement |
|---|---|
| FR-DAT-0200 | The store MUST live at `<project_root>/project.ndb` (SQLite). |
| FR-DAT-0201 | `EntityStore::open` failure MUST be surfaced via the runtime's error sink, not just logged to stderr. *(Tracked: fix/entitystore-error-surface.)* |
| FR-DAT-0202 | `EntityTable::save` failure MUST also be surfaced, returning `False` to the script AND emitting an error line. |

### 8.3 Forms (`.frm`)
| ID | Requirement |
|---|---|
| FR-DAT-0300 | A Form MUST be persisted as XML with `<Meta>` and a `<Widgets>` tree. |
| FR-DAT-0301 | Each widget MUST carry `type`, `name`, geometry, and per-type properties. |
| FR-DAT-0302 | An Activity references its forms via `<Form file="…"/>` OR `<Form>name.frm</Form>` (legacy text-content form). Both MUST be honoured. |

### 8.4 Activities (`.aba`)
| ID | Requirement |
|---|---|
| FR-DAT-0400 | An Activity MUST be persisted as XML with `<Meta>`, `<Forms>`, and `<Code><![CDATA[…]]></Code>`. |
| FR-DAT-0401 | The `Code` block MUST be treated as a single compilation unit; line numbers reported in errors MUST be relative to this block. |

### 8.5 Processes (`.prc`)
| ID | Requirement |
|---|---|
| FR-DAT-0500 | A Process MUST be persisted as BPMN 2.0 XML conformant to `OMG formal/2011-01-03`. |
| FR-DAT-0501 | Server step code MUST be carried in a Nexor-namespaced extension element under each `<task>` / `<scriptTask>`. |
| FR-DAT-0502 | Choice gateway conditions MUST be Nexor expressions stored on the outgoing `<sequenceFlow>`'s extension. |

---

## 9 · Package Format (`.nexor`)

| ID | Requirement |
|---|---|
| FR-PKG-0100 | The `.nexor` file MUST be a single UTF-8 XML document with a root `<Package version="1">`. |
| FR-PKG-0101 | The root MUST contain `<Meta>` (id, version, title, author, created), `<Files>`, and `<Signature>`. |
| FR-PKG-0102 | Each `<File>` element MUST carry `path`, `kind` (Activity / Form / Sheet / Process), `sha256` (hex), and the file body (CDATA for text; base64 for binary). |
| FR-PKG-0103 | The signature MUST be HMAC-SHA256 over the canonicalised body, using the project's signing key. |
| FR-PKG-0104 | All text bodies MUST be normalised to LF before hashing and embedding. (XML readers normalise on read; CRLF on write would break the hash.) |
| FR-PKG-0105 | Verification MUST fail with a descriptive error if any per-file SHA mismatches OR the signature fails. |

---

## 10 · External Interfaces

### 10.1 Core HTTP API
- Base URL: configurable, default `http://localhost:9000`.
- All bodies JSON unless noted; `.nexor` register endpoint accepts `application/xml`.
- All endpoints MUST return RFC-9457 problem details on error.

See `docs/api-reference.md` for the full endpoint table.

### 10.2 Project tree (filesystem)
```
<root>/
├─ <id>.nxproj            ← project descriptor
├─ project.ndb            ← SQLite entity store (created on first run)
├─ activities/<n>/<n>.aba
├─ activities/<n>/*.frm
├─ sheets/<id>/<id>.sht
├─ processes/<id>/<id>.prc
└─ dist/<id>-<version>.nexor
```

---

## 11 · Non-Functional Requirements

### 11.1 Performance
| ID | Requirement |
|---|---|
| NFR-PRF-0100 | A clean build of all four apps MUST complete in under 5 minutes on the reference dev box (Qt 5.14, mingw730_64, 8 cores). |
| NFR-PRF-0101 | Compiling and executing the largest sample (`order-approval`) MUST complete in under 1 second on first run, under 200 ms on warm runs. |
| NFR-PRF-0102 | Core MUST handle 100 concurrent entity GETs at p99 < 100 ms on the reference box. |
| NFR-PRF-0103 | Studio MUST open a 50-activity project in under 2 seconds. |

### 11.2 Reliability
| ID | Requirement |
|---|---|
| NFR-REL-0100 | A runtime error in a script MUST NOT crash Studio / Flux / Core. The host MUST surface the error and remain interactive. |
| NFR-REL-0101 | Saving an Activity / Form / Sheet / Process MUST be atomic — power loss mid-write MUST NOT corrupt the file (use temp + rename). |
| NFR-REL-0102 | The package registry on Core MUST survive a process kill — state is in SQLite. |

### 11.3 Build hygiene
| ID | Requirement |
|---|---|
| NFR-BLD-0100 | All four apps MUST compile with **zero warnings** under the project's default qmake config. |
| NFR-BLD-0101 | The language smoke harness (`tools/lang_smoke/`) MUST pass 100% of its cases on every commit to develop. |

### 11.4 Compatibility
| ID | Requirement |
|---|---|
| NFR-CMP-0100 | A `.nexor` package built with Studio 1.x MUST be readable by Core 1.y for all y ≥ x within major version 1. |
| NFR-CMP-0101 | Sheet schema migrations MUST be additive within major version 1 (no drops, no retypes). |

### 11.5 Security
| ID | Requirement |
|---|---|
| NFR-SEC-0100 | The signing key for `.nexor` packages MUST NOT be checked into source control. Operators MUST supply it via environment variable or a file outside the project tree. |
| NFR-SEC-0101 | Core MUST reject any package whose HMAC does not verify with the configured key. |
| NFR-SEC-0102 | HTTPS / authentication for Core's HTTP surface is the operator's responsibility (reverse proxy); Core MUST run cleanly behind one. |

### 11.6 Usability
| ID | Requirement |
|---|---|
| NFR-USA-0100 | Every error surfaced in Studio's output dock MUST be prefixed with `ERROR:` and coloured red. |
| NFR-USA-0101 | The dark IDE theme MUST be the default; a light theme MAY be added later. |
| NFR-USA-0102 | Keyboard shortcuts MUST follow widely-known conventions (`Ctrl+S`, `Ctrl+F5`, etc.). |

### 11.7 Maintainability
| ID | Requirement |
|---|---|
| NFR-MNT-0100 | The shared source tree (`nexor_studio/src/`) MUST gate widget / form-runner / process-engine sub-systems behind `NEXOR_HAS_*` defines so Core can build without GUI deps. |
| NFR-MNT-0101 | New language features MUST land with corresponding cases in `tools/lang_smoke/`. |

---

## 12 · Constraints

| ID | Constraint |
|---|---|
| CON-0100 | Qt version MUST be 5.14.x; Qt 6 migration is out of scope. |
| CON-0101 | C++ standard MUST be C++14 (the qmake `.pro` files set `CONFIG += c++14`). |
| CON-0102 | The docs site MUST stay on Astro 4.16 + Starlight 0.30 (zod compatibility). |
| CON-0103 | Core's database MUST be SQLite in 1.0. |

---

## 13 · Acceptance Criteria for 1.0

A 1.0 release candidate MUST satisfy all of the following:

1. ✅ All four apps compile with **zero warnings** under the reference Qt + mingw toolchain.
2. ✅ `tools/lang_smoke/` reports **0 failures** across all cases (currently 53/53).
3. ✅ `tools/check_form/` opens every sample form without error.
4. Each sample project (`hello-world`, `customer-crud`, `order-approval`) MUST:
   - Open in Studio without warnings.
   - Build to a `.nexor` whose HMAC verifies and whose every per-file SHA matches on re-read.
   - Register on Core, deploy via Command, run end-to-end in Flux.
5. The user-facing fixes called out by tracked tasks MUST be merged:
   - `fix/entitystore-error-surface` (silent Save failures → visible errors).
6. All disabled menu items MUST either be implemented OR explicitly documented as "1.1+ feature".
7. The documentation set under `docs/` MUST cover every public-facing concept (Language, Forms, Sheets, Processes, Publishing, Server-side, API reference).

---

## 14 · Traceability

Every requirement ID in this document is intended to be cross-referenced by:
- a test in `tools/lang_smoke/` or `tools/check_form/`,
- a sample under `docs/samples/`,
- and (for FR-CORE-* / FR-PKG-*) an endpoint or schema test.

When adding a requirement, also add or update its trace target.

---

## Appendix A · Glossary of file extensions
| Ext       | Meaning                                                       |
|-----------|---------------------------------------------------------------|
| `.nxproj` | Project descriptor (XML).                                     |
| `.aba`    | Atomic Activity — code + form list (XML with CDATA code).     |
| `.frm`    | Form definition (XML).                                        |
| `.sht`    | Sheet schema (XML).                                           |
| `.prc`    | Process — BPMN 2.0 XML.                                       |
| `.nexor`  | Signed package containing all of the above.                   |
| `.ndb`    | SQLite entity store (`project.ndb` per project).              |

## Appendix B · Reference samples
| Sample            | Demonstrates                                                  |
|-------------------|---------------------------------------------------------------|
| `hello-world`     | Smallest viable project: one activity, one form, one button.  |
| `customer-crud`   | Sheet-backed CRUD: `New / Save / Find / Delete`, list form.   |
| `order-approval`  | BPMN with Server step, Choice gateway, HumanTask, Final.      |
