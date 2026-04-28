# -----------------------------------------------------------------------------
# Generates Nexor-Platform-Architecture.pdf — the four Nexor apps
# (Studio, Command, Flux, Core) + the three end-user targets (Flux, Browser,
# Android), how they relate, and the full develop → administer → run pipeline.
# -----------------------------------------------------------------------------

from pathlib import Path

from reportlab.lib.pagesizes import A4
from reportlab.lib.styles    import getSampleStyleSheet, ParagraphStyle
from reportlab.lib           import colors
from reportlab.lib.units     import mm
from reportlab.lib.enums     import TA_LEFT
from reportlab.platypus      import (
    SimpleDocTemplate, Paragraph, Spacer, Preformatted,
    Table, TableStyle, PageBreak,
)


# ─── Styles ──────────────────────────────────────────────────────────────────
base = getSampleStyleSheet()

styles = {
    "Title":    ParagraphStyle("title",    parent=base["Title"],
                               fontName="Helvetica-Bold",
                               fontSize=22, leading=28,
                               textColor=colors.HexColor("#1a1a2e"),
                               spaceAfter=4),
    "Subtitle": ParagraphStyle("subtitle", parent=base["Normal"],
                               fontName="Helvetica",
                               fontSize=11, leading=15,
                               textColor=colors.HexColor("#666666"),
                               spaceAfter=20),
    "H1":       ParagraphStyle("h1",       parent=base["Heading1"],
                               fontName="Helvetica-Bold",
                               fontSize=16, leading=22,
                               textColor=colors.HexColor("#1e3a5f"),
                               spaceBefore=18, spaceAfter=8,
                               keepWithNext=True),
    "H2":       ParagraphStyle("h2",       parent=base["Heading2"],
                               fontName="Helvetica-Bold",
                               fontSize=12, leading=16,
                               textColor=colors.HexColor("#2a4a72"),
                               spaceBefore=10, spaceAfter=4,
                               keepWithNext=True),
    "Body":     ParagraphStyle("body",     parent=base["BodyText"],
                               fontName="Helvetica",
                               fontSize=10, leading=14,
                               textColor=colors.HexColor("#1a1a1a"),
                               spaceAfter=6,
                               alignment=TA_LEFT),
    "Bullet":   ParagraphStyle("bullet",   parent=base["BodyText"],
                               fontName="Helvetica",
                               fontSize=10, leading=14,
                               textColor=colors.HexColor("#1a1a1a"),
                               leftIndent=14,
                               bulletIndent=2,
                               spaceAfter=2),
    "Code":     ParagraphStyle("code",     parent=base["Code"],
                               fontName="Courier",
                               fontSize=8.5, leading=11,
                               textColor=colors.HexColor("#000000"),
                               backColor=colors.HexColor("#f4f4f8"),
                               borderColor=colors.HexColor("#cccccc"),
                               borderWidth=0.5,
                               borderPadding=6,
                               leftIndent=4, rightIndent=4,
                               spaceBefore=4, spaceAfter=8),
    "Note":     ParagraphStyle("note",     parent=base["BodyText"],
                               fontName="Helvetica-Oblique",
                               fontSize=10, leading=14,
                               textColor=colors.HexColor("#444444"),
                               backColor=colors.HexColor("#fff8e1"),
                               borderColor=colors.HexColor("#e0c060"),
                               borderWidth=0.5,
                               borderPadding=8,
                               leftIndent=4, rightIndent=4,
                               spaceBefore=4, spaceAfter=8),
}


# ─── Helpers ─────────────────────────────────────────────────────────────────
def P(text, style="Body"):                return Paragraph(text, styles[style])
def H1(text):                             return Paragraph(text, styles["H1"])
def H2(text):                             return Paragraph(text, styles["H2"])
def Code(text):                           return Preformatted(text, styles["Code"])
def Bullet(text):                         return Paragraph(f"• {text}", styles["Bullet"])
def NoteBox(text):                        return Paragraph(text, styles["Note"])
def Spacer8():                            return Spacer(1, 8)
def Spacer14():                           return Spacer(1, 14)


def TableBlock(rows, col_widths=None):
    if col_widths is None:
        col_widths = [55*mm, 110*mm]
    t = Table(rows, colWidths=col_widths, repeatRows=1)
    t.setStyle(TableStyle([
        ("BACKGROUND",     (0, 0), (-1, 0),  colors.HexColor("#1e3a5f")),
        ("TEXTCOLOR",      (0, 0), (-1, 0),  colors.white),
        ("FONTNAME",       (0, 0), (-1, 0),  "Helvetica-Bold"),
        ("FONTSIZE",       (0, 0), (-1, -1), 9),
        ("BOTTOMPADDING",  (0, 0), (-1, -1), 5),
        ("TOPPADDING",     (0, 0), (-1, -1), 5),
        ("LEFTPADDING",    (0, 0), (-1, -1), 6),
        ("RIGHTPADDING",   (0, 0), (-1, -1), 6),
        ("VALIGN",         (0, 0), (-1, -1), "TOP"),
        ("BACKGROUND",     (0, 1), (-1, -1), colors.HexColor("#f8f8fa")),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1),
                            [colors.HexColor("#f8f8fa"), colors.HexColor("#ffffff")]),
        ("LINEBELOW",      (0, 0), (-1, 0),  0.6, colors.HexColor("#1e3a5f")),
        ("LINEBELOW",      (0, 1), (-1, -1), 0.25, colors.HexColor("#e0e0e0")),
        ("BOX",            (0, 0), (-1, -1), 0.4, colors.HexColor("#cccccc")),
    ]))
    return t


# ─── Document content ────────────────────────────────────────────────────────
story = []

story += [
    Paragraph("Nexor — Platform Architecture", styles["Title"]),
    Paragraph("How Studio, Command, Core, and Flux fit together — and how a "
              "built project ends up running on Flux, the browser, and Android",
              styles["Subtitle"]),
]

# 1 ── The four Nexor apps + the three end-user targets
story += [
    H1("1 · The four Nexor apps + three end-user targets"),
    P("Nexor is a platform, not a single program.  Four named apps cover the "
      "developer / admin / end-user / server roles, and end-user activities "
      "can be reached from three host types."),
    Code(
"""┌───────────────────────────────────────────────────────────────────┐
│                      DEVELOPER TIME                               │
│              ┌──────────────────────────┐                         │
│              │      Nexor Studio        │  blue   icon            │
│              │      (IDE — desktop)     │                         │
│              │  • code editor           │                         │
│              │  • form designer         │                         │
│              │  • compiler              │                         │
│              │  • debugger              │                         │
│              └────────────┬─────────────┘                         │
│                           │ .nexor (signed)                       │
└───────────────────────────┼───────────────────────────────────────┘
                            ▼
┌───────────────────────────────────────────────────────────────────┐
│                      OPERATIONS TIME                              │
│              ┌──────────────────────────┐                         │
│              │      Nexor Command       │  orange icon            │
│              │  (admin — desktop + web) │                         │
│              │  • users / roles / perms │                         │
│              │  • package deployment    │                         │
│              │  • tenants / licensing   │                         │
│              │  • audit / monitoring    │                         │
│              └────────────┬─────────────┘                         │
│                           │ approve + deploy                      │
└───────────────────────────┼───────────────────────────────────────┘
                            ▼
┌───────────────────────────────────────────────────────────────────┐
│                       Nexor Core (server)         purple icon     │
│   package registry + entity DB + workflow engine + server VM      │
└──┬───────────────────┬─────────────────────┬──────────────────────┘
   │                   │                     │
   ▼                   ▼                     ▼
┌──────────┐    ┌──────────────┐    ┌──────────────┐
│   Flux   │    │   Browser    │    │   Android    │  green icon
│ (desktop │    │ (web SPA)    │    │   (app)      │  on all three
│ end-user)│    │              │    │              │
└──────────┘    └──────────────┘    └──────────────┘
                       END-USER TIME"""
    ),
    P("Three time-horizons, four apps, three host types, one bytecode."),
]

# 2 ── Roles
story += [
    H1("2 · Roles — who uses what"),
    TableBlock([
        ["App",            "Used by",              "Purpose"],
        ["Nexor Studio",   "Developers",
                           "Author Atomic and Process Activities, design forms, define Sheets, "
                           "compile, debug, build the .nexor package."],
        ["Nexor Command",  "IT administrators",
                           "Provision users, define roles + permissions, deploy / rollback "
                           ".nexor packages, manage tenants, license, audit and monitor."],
        ["Nexor Flux",     "End users (desktop)",
                           "Run published activities, view forms, drive workflows."],
        ["Nexor Core",     "Itself (no UI)",
                           "Hosts the entity database, the workflow engine, the package "
                           "registry, scheduled jobs, and server-side VM execution."],
    ], col_widths=[35*mm, 35*mm, 95*mm]),
    Spacer8(),
    P("End users can also reach the same published activities via a <b>browser</b> "
      "or <b>Android phone</b> — those are not separate apps, they are alternative "
      "host runtimes for the same .nexor package."),
]

# 3 ── One bytecode, multiple hosts
story += [
    H1("3 · Execution principle: one bytecode, multiple hosts"),
    P("The Studio compiler emits <b>one</b> artifact — the .nexor package — and "
      "<b>every</b> host (Flux, Command, Browser, Android, server-side Core) runs "
      "the same bytecode.  Each host adds four pieces on top:"),
    Code(
"""┌──────────────────────────────────────────────────────────┐
│ Per-host runtime layer (the only thing that varies)      │
│                                                          │
│   ┌──────────┐  ┌────────────┐  ┌─────────┐  ┌────────┐  │
│   │ Bytecode │  │ Stdlib     │  │ Form    │  │Network │  │
│   │   VM     │  │ bindings   │  │renderer │  │ + cache│  │
│   └──────────┘  └────────────┘  └─────────┘  └────────┘  │
└──────────────────────────────────────────────────────────┘
                         ▲ same .nexor in
─────────────────────────┴────────────────────────────────
          shared compiler / language / package format"""
    ),
    P("Single source of truth.  One compiler, one debugger, one set of language "
      "semantics.  The cost is implementing the VM three times (C++ for Flux / "
      "Command / Core, WASM-or-JS for browser, Kotlin for Android), but the VM "
      "is small (~3 000 LOC), so each port is a few weeks."),
]

# 4 ── Lifecycle
story += [
    H1("4 · Lifecycle: develop → administer → run"),
    Code(
"""┌──────────────┐   build      ┌────────────┐   publish     ┌────────────┐
│ Studio (IDE) │─────────────►│  .nexor    │──────────────►│ Nexor Core │
│              │  compiler    │  package   │  signed POST  │  package   │
│              │              │            │               │  registry  │
└──────────────┘              └────────────┘               └─────┬──────┘
                                                                 │
                                                  Pending review │
                                                                 ▼
                                                       ┌────────────────┐
                                                       │ Nexor Command  │
                                                       │  (admin app)   │
                                                       │  • review      │
                                                       │  • approve     │
                                                       │  • migrate DB  │
                                                       │  • deploy      │
                                                       └───────┬────────┘
                                                               │
                                              ┌────────────────┼─────────────────┐
                                              │                │                 │
                                              ▼                ▼                 ▼
                                       ┌────────────┐   ┌────────────┐    ┌────────────┐
                                       │   Flux     │   │  Browser   │    │  Android   │
                                       │  (desktop) │   │  (web SPA) │    │   (app)    │
                                       └────────────┘   └────────────┘    └────────────┘"""
    ),
    Bullet("<b>Developer (Studio)</b> — Build → produces .nexor.  Publish → uploads to "
           "Core's package registry as a <i>pending</i> version."),
    Bullet("<b>Administrator (Command)</b> — Reviews the pending package: change list, "
           "schema migration plan, new permissions introduced.  Approves → deployment "
           "pipeline runs migrations, swaps the live version, broadcasts notification."),
    Bullet("<b>End users (Flux / Browser / Android)</b> — Receive the new version "
           "automatically (auto-update on next launch, or live-reload over WebSocket)."),
    Spacer8(),
    P("This split — Studio publishes, Command deploys — is what keeps unauthorized "
      "code out of production: a developer cannot push code straight to live users "
      "without an admin's approval."),
]

# 5 ── The runtimes
story += [
    PageBreak(),
    H1("5 · The runtimes"),
    Code(
"""┌──────────────────────┬─────────────────────┬─────────────────────┬──────────────────┐
│        Server        │        Flux         │       Browser       │     Android      │
│      (Nexor Core)    │      (Desktop)      │       (Web SPA)     │      (App)       │
├──────────────────────┼─────────────────────┼─────────────────────┼──────────────────┤
│ VM        : C++      │ VM        : C++     │ VM        : WASM    │ VM     : Kotlin  │
│ Stdlib    : libc++   │ Stdlib    : libc++  │   (or pure JS port) │   (JVM)          │
│ DB        : Postgres │ Forms     : Qt      │ Stdlib : JS/Web API │ Stdlib : JDK     │
│ Forms     : —        │ Cache     : SQLite  │ Forms  : DOM/CSS    │ Forms  : Android │
│ Workflow  : durable  │ Net       : QtWS    │ Cache  : IndexedDB  │           Views  │
│ Scheduler : cron     │ Offline   : yes     │ Net    : fetch / WS │ Cache  : Room    │
│ Auth      : OAuth2   │                     │ Offline: yes (PWA)  │ Net    : OkHttp  │
│                      │                     │                     │ Push   : FCM     │
│                      │                     │                     │ Offline: yes     │
└──────────────────────┴─────────────────────┴─────────────────────┴──────────────────┘"""
    ),
    P("<b>Nexor Studio</b> and <b>Nexor Command</b> are themselves <i>Flux-like</i> "
      "runtimes (C++ VM + Qt) but with bundled developer-tooling / admin-tooling "
      "packages baked in — they share the same shell.  That keeps a single C++ "
      "codebase running every desktop component."),
    TableBlock([
        ["Layer",            "What it does"],
        ["Bytecode VM",      "Stack machine, ~30 procedural opcodes + ~15 ERP/transaction opcodes. "
                             "Same semantics across all hosts."],
        ["Stdlib bindings",  "Native impls of nexor.lang, nexor.io, nexor.erp wired to each host's OS APIs."],
        ["Form renderer",    "Reads each form's XML widget tree and produces native widgets — Qt / DOM / Android Views."],
        ["Network + cache",  "REST/WebSocket to Core plus a local entity cache so the app works offline."],
    ], col_widths=[40*mm, 125*mm]),
]

# 6 ── Nexor Command in detail
story += [
    H1("6 · Nexor Command — the admin app"),
    P("Command is the operations counterpart to Studio.  It is itself a Flux-shape "
      "runtime (C++ VM + Qt forms) with a bundle of pre-installed admin packages "
      "that ship with every Nexor install."),
    H2("Built-in admin modules"),
    TableBlock([
        ["Module",             "Manages"],
        ["Users",              "User accounts: create, deactivate, reset password, force MFA, "
                               "tie to LDAP / AD."],
        ["Roles",              "Named role definitions: Sales Manager, Accountant, Warehouse Worker, …"],
        ["Permissions",        "Granular permission strings (Sales.Approve, Invoice.Void, …) "
                               "discovered automatically from deployed packages.  Admin assigns "
                               "permissions to roles, roles to users."],
        ["Packages",           "Pending / staged / live .nexor versions.  Diff viewer, schema "
                               "migration preview, deploy / rollback / pin actions."],
        ["Tenants",            "Multi-tenant SaaS deployments — provision a customer company, "
                               "isolate its data, set its quotas."],
        ["Licensing",          "Seat usage vs. licensed seats, expiration dates, feature flags."],
        ["Audit log",          "Searchable, filterable timeline of who did what, when, on which "
                               "tenant, with diffs."],
        ["Monitoring",         "Live dashboards for active sessions, RPC latency, workflow "
                               "throughput, error rates."],
        ["Backups / restore",  "Trigger manual backups, browse / restore snapshots."],
        ["Integrations",       "Configure outbound webhooks, inbound SSO, SMTP, file storage, FCM."],
    ], col_widths=[40*mm, 125*mm]),
    Spacer8(),
    H2("Architectural insight"),
    P("These admin modules are themselves <b>written in the Nexor language</b> (Atomic "
      "Activities, Sheets, Forms — exactly like a customer ERP module).  This eats "
      "our own dog food: if our language can't express user management, it can't "
      "express your accounting module either."),
    P("Concretely, Command is implemented as:"),
    Bullet("The same desktop shell as Flux (C++ VM + Qt + form renderer)."),
    Bullet("A bundle of pre-installed packages: nexor.admin.users, nexor.admin.roles, "
           "nexor.admin.packages, nexor.admin.audit, etc."),
    Bullet("Each admin package targets <b>admin entities</b> exposed by Nexor Core "
           "(User, Role, Permission, PackageVersion, AuditEvent, etc.) — entities "
           "ordinary users cannot see, gated by the role <b>nexor.administrator</b>."),
    Bullet("A web build (PWA) is also produced from the same admin packages, so "
           "an on-call admin can intervene from a browser when the desktop tool "
           "isn't available."),
    H2("Where Command lives"),
    P("Command runs on the IT admin's machine (Windows / Mac / Linux desktop).  "
      "It connects to Nexor Core over the same REST + WebSocket protocol as any "
      "other client, but its session token carries the <b>nexor.administrator</b> "
      "role, which unlocks the /api/v1/admin/* endpoints and the admin entities."),
]

# 7 ── Form rendering
story += [
    PageBreak(),
    H1("7 · Form rendering — same form, three native UIs"),
    P("Every widget type in the palette has a fixed mapping per host:"),
    TableBlock([
        ["Nexor type",     "Flux (Qt) / Browser (DOM) / Android (Views)"],
        ["Button",         "QPushButton / &lt;button&gt; / Button"],
        ["Label",          "QLabel / &lt;span&gt; or &lt;label&gt; / TextView"],
        ["TextBox",        "QLineEdit / &lt;input type=text&gt; / EditText"],
        ["TextArea",       "QPlainTextEdit / &lt;textarea&gt; / EditText (multiline)"],
        ["CheckBox",       "QCheckBox / &lt;input type=checkbox&gt; / CheckBox"],
        ["RadioButton",    "QRadioButton / &lt;input type=radio&gt; / RadioButton"],
        ["ComboBox",       "QComboBox / &lt;select&gt; / Spinner"],
        ["ListBox",        "QListWidget / &lt;ul&gt; (or virtual list) / RecyclerView"],
        ["DateTimePicker", "QDateTimeEdit / &lt;input type=datetime-local&gt; / DatePickerDialog"],
        ["ProgressBar",    "QProgressBar / &lt;progress&gt; / ProgressBar"],
        ["TabControl",     "QTabWidget / tab &lt;div&gt;s / TabLayout + ViewPager2"],
        ["GroupBox",       "QGroupBox / &lt;fieldset&gt; / MaterialCardView"],
        ["PictureBox",     "QLabel(pixmap) / &lt;img&gt; / ImageView"],
    ], col_widths=[35*mm, 130*mm]),
    Spacer8(),
    P("The form's &lt;Geometry&gt; + per-widget x/y/w/h/anchor become:"),
    Bullet("<b>Flux / Studio / Command</b> — absolute positioning inside a QWidget, "
           "anchor flags as Qt size policies."),
    Bullet("<b>Browser</b> — CSS absolute positioning; @media rules driven by anchor flags."),
    Bullet("<b>Android</b> — ConstraintLayout with constraint sides keyed to anchor flags."),
]

# 8 ── Activity placement
story += [
    H1("8 · Where each activity actually runs"),
    P("Each Atomic Activity carries a RunsOn annotation that the compiler enforces:"),
    Code(
"""[Activity(RunsOn := ServerOnly,  RequiresPermission := "Sales.Approve")]
Public Sub ApproveOrder(orderId As Long)
    ...
End Sub

[Activity(RunsOn := ClientOnly)]
Public Sub txtSearch_Change()
    Form.lblHint.Text = "Type at least 3 chars"
End Sub

[Activity(RunsOn := Either)]
Public Function FormatTotal(amt As Decimal) As String
    Return Money.Format(amt, currentUser.Locale)
End Function"""
    ),
    TableBlock([
        ["Annotation",      "Where the bytecode runs / typical use"],
        ["ServerOnly",      "Nexor Core. DB writes, business rules, scheduled jobs, sensitive logic. "
                            "Client gets an RPC stub that calls the server."],
        ["ClientOnly",      "Flux / Browser / Android. UI events, local validation, animations."],
        ["Either (default)","Wherever it's invoked. Pure utility functions, formatting, math."],
    ], col_widths=[35*mm, 130*mm]),
    Spacer8(),
    P("Admin activities (used by Command) carry an extra <b>RequiresRole := \"nexor.administrator\"</b> "
      "annotation; the compiler refuses to call them from non-admin contexts."),
]

# 9 ── Server side
story += [
    H1("9 · Server side (Nexor Core)"),
    Code(
"""┌──────────────────────────────────────────────────────────────┐
│                         Nexor Core                           │
│                                                              │
│  ┌───────────┐  ┌───────────┐  ┌──────────┐  ┌────────────┐  │
│  │ HTTP / WS │  │   Auth    │  │  Tenant  │  │   Audit    │  │
│  │  gateway  │─►│  (OAuth2  │─►│  router  │─►│   logger   │  │
│  └───────────┘  │   + JWT)  │  └──────────┘  └────────────┘  │
│                 └───────────┘                                │
│  ┌────────────────────────────────────────────────────────┐  │
│  │             Server-side bytecode VM                    │  │
│  └────────────────────────────────────────────────────────┘  │
│       │              │              │             │          │
│       ▼              ▼              ▼             ▼          │
│  ┌─────────┐   ┌────────────┐  ┌──────────┐  ┌────────────┐  │
│  │ Package │   │ Entity ORM │  │ Workflow │  │ Scheduler  │  │
│  │registry │   │ (Postgres) │  │  engine  │  │  (cron)    │  │
│  └─────────┘   └────────────┘  └──────────┘  └────────────┘  │
└──────────────────────────────────────────────────────────────┘"""
    ),
    TableBlock([
        ["Subsystem",          "Purpose"],
        ["HTTP/WS gateway",    "REST request/response and WebSocket live updates."],
        ["Auth",               "OAuth2 + signed JWT carrying userId, tenantId, role list."],
        ["Tenant router",      "Auto-applies WHERE tenant_id = :t to every entity query."],
        ["Package registry",   "Stores .nexor versions (pending / staged / live), manages migrations + rollback."],
        ["VM",                 "Runs [ServerOnly] activities, returns results."],
        ["Entity ORM",         "Maps Sheets to Postgres tables; audit, optimistic concurrency, soft-delete."],
        ["Workflow engine",    "Persistent state machines for Process Activities; survives restarts."],
        ["Scheduler",          "Triggers timer-based activities (cron-style)."],
    ], col_widths=[40*mm, 125*mm]),
]

# 10 ── Wire protocols
story += [
    H1("10 · Wire protocols"),
    Code(
"""# End-user endpoints (any client with a valid token)
Client  ────►  POST /api/v1/auth/login              ◄────  { token }
Client  ────►  GET  /api/v1/packages                ◄────  [{id, version, hash}, …]
Client  ────►  GET  /api/v1/packages/<id>/<version> ◄────  .nexor (cacheable)
Client  ────►  GET  /api/v1/entities/Customer?...   ◄────  [{...}, …]
Client  ────►  POST /api/v1/entities/Customer       ◄────  { id, rowVersion }
Client  ────►  POST /api/v1/rpc/<package>/<sub>     ◄────  result | error
                  body = JSON-encoded args

# Admin endpoints (Command only — token must carry nexor.administrator)
Command ────►  POST /api/v1/admin/users
Command ────►  PATCH /api/v1/admin/users/:id
Command ────►  POST /api/v1/admin/roles/:id/permissions
Command ────►  POST /api/v1/admin/packages/:id/deploy   { version, runMigrations }
Command ────►  POST /api/v1/admin/packages/:id/rollback { toVersion }
Command ────►  GET  /api/v1/admin/audit?tenant=…&from=…&to=…
Command ────►  GET  /api/v1/admin/health
Command ────►  GET  /api/v1/admin/license

# WebSocket (any client)
WebSocket  ◄►  /api/v1/stream
   • package-published / package-deprecated
   • entity-changed (live updates)
   • workflow-step  (push notifications for human tasks)
   • admin-event    (deployments, role changes — Command only)"""
    ),
    P("Two transports cover everything: REST for request/response and WebSocket for live updates.  "
      "Admin endpoints sit under /api/v1/admin/* and are gated by the <b>nexor.administrator</b> "
      "role at the gateway."),
]

# 11 ── Offline & sync
story += [
    H1("11 · Offline &amp; data sync"),
    P("Mobile and browser users cannot assume connectivity.  Every host (including "
      "Command for offline admin reads) follows the same model:"),
    Bullet("1. <b>First open after publish</b> — client downloads the relevant .nexor "
           "and the entities it needs (filtered by tenant + permissions) into the "
           "local store (SQLite / IndexedDB / Room)."),
    Bullet("2. <b>While online</b> — reads go to the server first with the cache as "
           "fallback; writes go straight through."),
    Bullet("3. <b>While offline</b> — reads come from the cache; writes are appended "
           "to a <i>pending-ops</i> queue with their rowVersion."),
    Bullet("4. <b>On reconnect</b> — pending ops replay against the server.  Server "
           "checks rowVersion: matches → apply, broadcast change.  Mismatched → "
           "return Conflict.  Client opens a merge UI generated from the entity "
           "schema (the same form, with red diff badges on changed fields)."),
]

# 12 ── Web specifics
story += [
    PageBreak(),
    H1("12 · Web specifics"),
    Bullet("<b>Web SPA</b> served from Nexor Core at /app/ for end users and "
           "/admin/ for the Command web build."),
    Bullet("<b>VM</b> — WASM (the C++ VM compiled with Emscripten, ~200 KB gzipped) "
           "with a TypeScript JS port as fallback."),
    Bullet("<b>Form renderer</b> — generates HTML elements with absolute positioning "
           "inside a position:relative form root.  Anchor flags translate to CSS."),
    Bullet("<b>Storage</b> — entities cached in IndexedDB; small key/value state in localStorage."),
    Bullet("<b>PWA manifest</b> — installable on desktop and mobile browsers, runs offline."),
    Bullet("<b>Push</b> — Web Push API for workflow notifications."),
]

# 13 ── Android specifics
story += [
    H1("13 · Android specifics"),
    Bullet("<b>App shell</b> — native Kotlin app, ~3–5 MB, loads .nexor packages from "
           "Nexor Core or from a local getFilesDir()/packages/ folder."),
    Bullet("<b>VM</b> — Kotlin port of the spec, JIT'd by the JVM/ART."),
    Bullet("<b>Form renderer</b> — ConstraintLayout per form; anchor flags map directly "
           "to constraint sides."),
    Bullet("<b>Cache</b> — Room (SQLite) for entities; encrypted via "
           "EncryptedSharedPreferences for credentials."),
    Bullet("<b>Push</b> — FCM.  Workflow step assigned → server sends FCM → app shows "
           "notification → tap opens the form for that step."),
    Bullet("<b>Offline</b> — full read/write from the cache; queue replays on reconnect."),
    Spacer8(),
    P("iOS would follow the same shape (Swift VM port + UIKit/SwiftUI form renderer); "
      "saved for after Android."),
]

# 14 ── End-to-end build order
story += [
    H1("14 · End-to-end build order"),
    P("If you want a concrete path from \"today\" to a full four-app platform on "
      "all three host types:"),
    Bullet("1. <b>Lexer / parser / tree-walking interpreter</b> in Studio (week 1–2)."),
    Bullet("2. <b>C++ bytecode VM</b> in Flux + Core (week 3–4)."),
    Bullet("3. <b>Package format + Publish endpoint + Package registry</b> on Core (week 5)."),
    Bullet("4. <b>Server-side activity execution + RPC</b> ([ServerOnly] runs on Core) (week 6)."),
    Bullet("5. <b>Sheets / entity ORM</b> on Core, REST CRUD endpoints (week 7–8)."),
    Bullet("6. <b>Nexor Command shell</b> = Flux shell + the first admin module "
           "(<i>nexor.admin.users</i>) — proves the dogfood loop (week 9–10)."),
    Bullet("7. Remaining admin modules: roles, permissions, packages, audit (week 11–12)."),
    Bullet("8. <b>Web SPA shell + WASM VM + DOM form renderer</b> (week 13–16)."),
    Bullet("9. <b>Offline cache + sync</b> (Flux + Web) (week 17–18)."),
    Bullet("10. <b>Android app + Kotlin VM + ConstraintLayout renderer</b> (week 19–22)."),
    Bullet("11. <b>Workflow engine + push notifications</b> (week 23–26)."),
    Bullet("12. <b>Reports + scheduling</b> — last, because they sit on top of the rest."),
    Spacer8(),
    NoteBox(
        "Steps 1–4 give you \"build, publish, run on Flux + on the server\".  "
        "Step 6 unlocks Command (the admin loop).  Step 8 unlocks the browser.  "
        "Step 10 unlocks Android.  By step 11 you have a real ERP platform."),
    Spacer8(),
    P("The single architectural commitment that makes the whole thing possible is "
      "<b>the universal bytecode runtime</b>: one compiler, one package format, "
      "one set of language semantics — and four host adapters (Flux desktop, Command "
      "desktop, Browser, Android) that each give the runtime a face, plus a "
      "server-side host (Core) that runs the same bytecode behind the API gateway."),
]


# ─── Build PDF ───────────────────────────────────────────────────────────────
def build():
    out = Path.home() / "Desktop" / "Nexor-Platform-Architecture.pdf"
    out.parent.mkdir(parents=True, exist_ok=True)
    doc = SimpleDocTemplate(
        str(out),
        pagesize=A4,
        leftMargin=20*mm, rightMargin=20*mm,
        topMargin=18*mm,  bottomMargin=18*mm,
        title="Nexor — Platform Architecture",
        author="Nexor / Claude",
    )

    def footer(canvas, _doc):
        canvas.saveState()
        canvas.setFont("Helvetica", 8)
        canvas.setFillColor(colors.HexColor("#888888"))
        canvas.drawString(20*mm, 12*mm, "Nexor — Platform Architecture")
        canvas.drawRightString(A4[0] - 20*mm, 12*mm,
                               f"Page {canvas.getPageNumber()}")
        canvas.restoreState()

    doc.build(story, onFirstPage=footer, onLaterPages=footer)
    print(f"Wrote: {out}")


if __name__ == "__main__":
    build()
