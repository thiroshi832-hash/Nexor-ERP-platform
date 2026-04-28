---
title: 'Getting started'
description: 'Build all four apps and walk through the publish loop.'
---

This page walks you from a fresh Windows machine to a running Nexor stack with
all four apps talking to each other.

## 1. Prerequisites

| What | Version | Where |
|---|---|---|
| **Qt** | 5.14.0 | `C:\Qt\Qt5.14.0\5.14.0\mingw73_64\` |
| **MinGW** | 7.3.0 (64-bit) | `C:\Qt\Qt5.14.0\Tools\mingw730_64\` |
| **Git** | any | for cloning |
| **Python** | 3.x with Pillow + reportlab | only needed to regenerate icons / PDFs |

If you installed Qt with the Maintenance Tool the toolchain is already in place.

## 2. Clone and build

```cmd
git clone https://github.com/your-org/nexor.git
cd nexor
```

The repo is a Qt **subdirs** project (`Nexor.pro` at the root) that builds all
four apps in dependency order:

```cmd
set PATH=C:\Qt\Qt5.14.0\5.14.0\mingw73_64\bin;C:\Qt\Qt5.14.0\Tools\mingw730_64\bin;%PATH%
qmake Nexor.pro
mingw32-make -j8
```

After a clean build, four binaries land under `bin/`:

```
bin/
  NexorStudio.exe     # 🟦 IDE
  NexorCore.exe       # 🟪 backend
  NexorCommand.exe    # 🟧 admin
  NexorFlux.exe       # 🟩 end-user runtime
  Qt5Core.dll, Qt5Gui.dll, Qt5Widgets.dll, Qt5Network.dll, Qt5Sql.dll, Qt5Xml.dll
  platforms/qwindows.dll
  sqldrivers/qsqlite.dll
  …
```

You can also build a single app:

```cmd
cd nexor_studio
qmake nexor_studio.pro
mingw32-make -j8
```

## 3. First run end-to-end

This is the canonical "did everything wire up" smoke test. You'll publish a
package from Studio, deploy it from Command, and run it from Flux.

### 3a. Start Core

```cmd
bin\NexorCore.exe --port 7421 --admin-token devtoken
```

Output:

```
NexorCore listening on http://0.0.0.0:7421
  GET  /api/v1/health
  GET  /api/v1/packages
  POST /api/v1/packages
  GET  /api/v1/packages/:id/:version
  GET  /api/v1/admin/packages[?status=…]
  POST /api/v1/admin/packages/:id/:version/deploy
  POST /api/v1/admin/packages/:id/:version/rollback
  POST /api/v1/rpc/:package/:sub
  GET  /api/v1/entities/:sheet[/:id]
  POST /api/v1/entities/:sheet
  PATCH/DELETE /api/v1/entities/:sheet/:id
  POST /api/v1/processes/:package/:process/start
  POST /api/v1/processes/:instance/resume
  GET  /api/v1/processes[?status=...]
  GET  /api/v1/processes/:instance
```

Confirm it answers:

```cmd
curl http://localhost:7421/api/v1/health
{"service":"NexorCore","uptime":3,"version":"0.1.0"}
```

### 3b. Build a package in Studio

1. Launch `bin\NexorStudio.exe`.
2. **File → New Project…** → name it `HelloWorld`. Pick a folder on disk.
3. Right-click **Atomic Activities → New Atomic Activity…** → id `Hi`.
4. Open the auto-generated `Hi_main.frm`, drop a Button on it, set `Text = "Hi"`.
5. Double-click the button → it inserts a `Sub btnNew_Click` stub. Replace it with:

   ```vbnet
   Sub Button1_Click()
       MsgBox "Hello from Nexor!"
   End Sub
   ```

6. **F5** runs the form locally — confirm the button works.
7. **Build → Publishing Settings…** — set:
   - Core URL: `http://localhost:7421`
   - Admin token: `devtoken`
   - Signing key: leave blank (Core was started without `--signing-key`)
8. **Build → Build Package…** (`Ctrl+B`) — accept the default `0.1.0`.
9. **Build → Publish to Core…** (`Ctrl+Shift+P`) — confirm Core's URL.

The OUTPUT pane should show `Published HelloWorld-0.1.0.nexor → HTTP 200`.

### 3c. Deploy from Command

1. Launch `bin\NexorCommand.exe`.
2. **File → Settings…** — set Core URL + admin token to match Studio.
3. The package list shows `HelloWorld 0.1.0` with status `pending`.
4. Select it and click **▲ Deploy**. Status flips to `live`.
5. Open the **Audit** tab — you'll see two events: `publish` and `deploy`.

### 3d. Install and run from Flux

1. Launch `bin\NexorFlux.exe`.
2. **File → Settings…** — set Core URL.
3. The catalog shows `HelloWorld 0.1.0` as `AVAILABLE`. Select it and click **⤓ Install**.
4. After install it flips to `INSTALLED`. Click **▶ Run**.
5. Pick `Hi_main.frm` from the Activity Picker → click **▶ Run Form** → click your button → MessageBox fires.

You've gone from source code to a published, deployed, end-user-runnable app on
one machine in five minutes.

## 4. The four data roots

Each app keeps its state in a different place. Knowing this saves debugging time:

| App | Data root | Files |
|---|---|---|
| Core    | `%APPDATA%\Nexor\Core\` (overrideable via `--data`) | `core.db`, `core_entities.db`, `core_processes.db`, `packages/<id>/<ver>.nexor` |
| Flux    | `%APPDATA%\NexorFlux\` (Qt's `AppDataLocation`)     | `packages/<id>/<ver>.nexor`, `extracted/<id>/<ver>/` |
| Studio  | per-project, anywhere on disk you choose            | `<id>.pro`, `activities/`, `forms/`, `sheets/`, `processes/`, `dist/` |
| Command | `%APPDATA%\Nexor\NexorCommand\`                     | `QSettings` only — UI state, no data |

To wipe Core's state and start over: shut it down, delete its data root, restart.

## 5. Where to read next

- **[Language Reference](language-reference.md)** — what you can write inside a `Sub` body.
- **[Forms Guide](forms.md)** — every widget, every event, the `Form` bridge.
- **[Sheets & Entities](sheets.md)** — define schemas, query rows.
- **[Processes & BPMN](processes.md)** — model long-running workflows.
