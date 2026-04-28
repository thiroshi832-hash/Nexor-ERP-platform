# -----------------------------------------------------------------------------
# Generates Nexor-Language-Architecture.pdf from the in-memory document below.
# Uses ReportLab's Platypus.  Run once, output goes to the Desktop.
# -----------------------------------------------------------------------------

from pathlib import Path

from reportlab.lib.pagesizes  import A4
from reportlab.lib.styles     import getSampleStyleSheet, ParagraphStyle
from reportlab.lib            import colors
from reportlab.lib.units      import mm
from reportlab.lib.enums      import TA_LEFT
from reportlab.platypus       import (
    SimpleDocTemplate, Paragraph, Spacer, Preformatted,
    Table, TableStyle, PageBreak, KeepTogether,
)


# ─── Styles ──────────────────────────────────────────────────────────────────
base = getSampleStyleSheet()

styles = {
    "Title":      ParagraphStyle("title",      parent=base["Title"],
                                 fontName="Helvetica-Bold",
                                 fontSize=22, leading=28,
                                 textColor=colors.HexColor("#1a1a2e"),
                                 spaceAfter=4),
    "Subtitle":   ParagraphStyle("subtitle",   parent=base["Normal"],
                                 fontName="Helvetica",
                                 fontSize=11, leading=15,
                                 textColor=colors.HexColor("#666666"),
                                 spaceAfter=20),
    "H1":         ParagraphStyle("h1",         parent=base["Heading1"],
                                 fontName="Helvetica-Bold",
                                 fontSize=16, leading=22,
                                 textColor=colors.HexColor("#1e3a5f"),
                                 spaceBefore=18, spaceAfter=8,
                                 keepWithNext=True),
    "H2":         ParagraphStyle("h2",         parent=base["Heading2"],
                                 fontName="Helvetica-Bold",
                                 fontSize=12, leading=16,
                                 textColor=colors.HexColor("#2a4a72"),
                                 spaceBefore=10, spaceAfter=4,
                                 keepWithNext=True),
    "Body":       ParagraphStyle("body",       parent=base["BodyText"],
                                 fontName="Helvetica",
                                 fontSize=10, leading=14,
                                 textColor=colors.HexColor("#1a1a1a"),
                                 spaceAfter=6,
                                 alignment=TA_LEFT),
    "Bullet":     ParagraphStyle("bullet",     parent=base["BodyText"],
                                 fontName="Helvetica",
                                 fontSize=10, leading=14,
                                 textColor=colors.HexColor("#1a1a1a"),
                                 leftIndent=14,
                                 bulletIndent=2,
                                 spaceAfter=2),
    "Code":       ParagraphStyle("code",       parent=base["Code"],
                                 fontName="Courier",
                                 fontSize=9, leading=12,
                                 textColor=colors.HexColor("#000000"),
                                 backColor=colors.HexColor("#f4f4f8"),
                                 borderColor=colors.HexColor("#cccccc"),
                                 borderWidth=0.5,
                                 borderPadding=6,
                                 leftIndent=4, rightIndent=4,
                                 spaceBefore=4, spaceAfter=8),
    "Note":       ParagraphStyle("note",       parent=base["BodyText"],
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
def P(text, style="Body"):
    return Paragraph(text, styles[style])

def H1(text):  return Paragraph(text, styles["H1"])
def H2(text):  return Paragraph(text, styles["H2"])

def Code(text):
    return Preformatted(text, styles["Code"])

def Bullet(text):
    return Paragraph(f"• {text}", styles["Bullet"])

def NoteBox(text):
    return Paragraph(text, styles["Note"])

def Spacer8():  return Spacer(1, 8)
def Spacer14(): return Spacer(1, 14)


def TableBlock(rows, col_widths=None):
    """Two-column table styled like the document."""
    if col_widths is None:
        col_widths = [55*mm, 110*mm]
    t = Table(rows, colWidths=col_widths, repeatRows=1)
    t.setStyle(TableStyle([
        ("BACKGROUND",  (0, 0), (-1, 0),  colors.HexColor("#1e3a5f")),
        ("TEXTCOLOR",   (0, 0), (-1, 0),  colors.white),
        ("FONTNAME",    (0, 0), (-1, 0),  "Helvetica-Bold"),
        ("FONTSIZE",    (0, 0), (-1, -1), 9),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
        ("TOPPADDING",    (0, 0), (-1, -1), 5),
        ("LEFTPADDING",   (0, 0), (-1, -1), 6),
        ("RIGHTPADDING",  (0, 0), (-1, -1), 6),
        ("VALIGN",      (0, 0), (-1, -1), "TOP"),
        ("BACKGROUND",  (0, 1), (-1, -1), colors.HexColor("#f8f8fa")),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1),
                        [colors.HexColor("#f8f8fa"), colors.HexColor("#ffffff")]),
        ("LINEBELOW",   (0, 0), (-1, 0),  0.6, colors.HexColor("#1e3a5f")),
        ("LINEBELOW",   (0, 1), (-1, -1), 0.25, colors.HexColor("#e0e0e0")),
        ("BOX",         (0, 0), (-1, -1), 0.4, colors.HexColor("#cccccc")),
    ]))
    return t


# ─── Document content ────────────────────────────────────────────────────────
story = []

story += [
    Paragraph("Nexor — Language System Architecture", styles["Title"]),
    Paragraph("Design overview for the Nexor ERP platform language",
              styles["Subtitle"]),
]

# ── 1. Philosophy
story += [
    H1("1 · Language design philosophy"),
    P("<b>Optional static typing on top of VB-style procedural syntax.</b>"),
    Code(
"""Dim count                         ' variant — like VB Script
Dim customers As Customer[]       ' typed — like VB6 / .NET
Function NetTotal(o As Order) As Decimal
    Return o.Subtotal - o.Discount
End Function"""
    ),
    P("Why optional-static:"),
    Bullet("<b>Citizen developers</b> (typical ERP users) get the soft on-ramp of variants."),
    Bullet("<b>Senior developers</b> get type-checked code, IntelliSense, refactoring."),
    Bullet("Compiler can emit faster bytecode when types are known."),
    Bullet("Matches VB6 exactly — the dialect your users will recognize."),
]

# ── 2. Type system
story += [
    H1("2 · Type system"),
    TableBlock([
        ["Tier", "Types and notes"],
        ["Primitives", "Boolean, Integer, Long, Decimal, Double, String, Date, Variant.\n"
                       "Decimal is mandatory for ERP — never use Double for money."],
        ["Containers", "Array<T>, List<T>, Dictionary<K,V>, Set<T>. First-class generics."],
        ["Records",    "User-defined Type ... End Type. Value semantics."],
        ["Entities",   "Sheet-defined business objects. Reference semantics, persisted, audited."],
        ["References", "Object, late-bound — for interop / scripts."],
        ["Special",    "Nothing, Null, Empty. Three-valued like SQL — needed for nullable DB columns."],
    ], col_widths=[35*mm, 130*mm]),
    Spacer8(),
    P("The killer feature is <b>Entities</b>: schema-aware first-class types backed "
      "by the ERP store.  They're declared in the project tree under "
      "<font face='Courier'>Sheets</font>."),
]

# ── 3. Compilation model
story += [
    H1("3 · Compilation model"),
    Code(
""".aba / .frm  ──►  Lexer  ──►  Parser  ──►  AST
                                            │
                              Semantic + Schema check
                                            │
                                  IL (typed bytecode)
                                            │
                                .abx / .frx (compiled)
                                            │
                              Package builder ──► .nexor"""
    ),
    Bullet("<b>Bytecode VM</b>, not tree-walking, not native.  Sweet spot of speed, "
           "debugability, and portability between Core (server) and Flux (client)."),
    Bullet("<b>.abx / .frx</b> are the compiler outputs (already in your file-extension scheme)."),
    Bullet("<b>.nexor package</b> bundles compiled activities + form definitions + entity "
           "schemas + resources, signed and version-stamped, deployed by Nexor Command."),
    Spacer8(),
    P("The VM is small enough (~20–30 opcodes for the procedural core, plus ~15 for "
      "entity/transaction primitives) that you can implement it in a few thousand lines "
      "of C++ and ship it inside both Core and Flux."),
]

# ── 4. Three-tier runtime
story += [
    H1("4 · Three-tier runtime"),
    Code(
"""┌────────────┐   bytecode   ┌────────────┐
│   Studio   │─compiles────►│   .nexor   │
│   (IDE)    │              │  package   │
└────────────┘              └─────┬──────┘
                                  │ deployed by Command
              ┌───────────────────┴───────────────────┐
              │                                       │
        ┌─────▼──────┐                          ┌─────▼──────┐
        │ Nexor Core │                          │ Nexor Flux │
        │  (server)  │◄────── REST / WS ───────►│  (client)  │
        │            │                          │            │
        │ • DB       │                          │ • Forms    │
        │ • Server-  │                          │ • Client-  │
        │   side     │                          │   side     │
        │   activity │                          │   activity │
        │ • Workflow │                          │ • Reports  │
        │   engine   │                          │   render   │
        └────────────┘                          └────────────┘"""
    ),
    P("Each Atomic Activity has a <b>RunsOn</b> attribute:"),
    Code(
"""[Activity(RunsOn := ServerOnly, RequiresPermission := "Sales.Approve")]
Public Sub ApproveOrder(orderId As Long)
    ...
End Sub"""
    ),
    P("The compiler enforces this — a ClientOnly activity can't directly access "
      "entities; it has to call a ServerOnly activity by RPC."),
]

# ── 5. ERP-specific primitives
story += [
    PageBreak(),
    H1("5 · ERP-specific primitives"),
    P("These are what make the language an <i>ERP</i> language rather than a general "
      "scripting language."),

    H2("Entities + queries"),
    Code(
"""' Schema declared in a Sheet (designer-editable XML on disk):
Entity Customer
    Id        As Long       Key
    Name      As String     Required, MaxLength := 200
    Email     As String     Pattern := "^[^@]+@[^@]+$"
    Balance   As Decimal    Computed
    Created   As Date       Audit
End Entity

' Query syntax:
Dim due = From c In Customers
          Where c.Balance > 0 And c.Region = currentUser.Region
          OrderBy c.Balance Descending
          Take 50

' Or lookup form:
Dim cust = Customers.Find(orderId)   ' nullable
cust.Email = "new@example.com"
cust.Save()                           ' persists + audit-logs"""
    ),
    P("The compiler validates field references against the entity schema at compile "
      "time.  This is the single biggest productivity win for ERP devs."),

    H2("Transactions"),
    Code(
"""Begin Transaction
    Dim inv = Invoices.New()
    inv.Customer = cust
    inv.Total    = order.Total
    inv.Save()

    cust.Balance = cust.Balance + inv.Total
    cust.Save()
Commit
' On any error inside, the runtime auto-rolls back and re-raises."""
    ),

    H2("Process Activities (workflow / orchestration)"),
    P("This is where Nexor's <b>ProcessActivities</b> node earns its keep.  A Process "
      "Activity is a long-running, persisted state machine:"),
    Code(
"""Process OrderFulfillment
    Trigger On Orders.Created

    Step Validate As ServerOnly
        If order.Total > 10000 Then
            Goto NeedsApproval
        Else
            Goto Pick
        End If

    Step NeedsApproval As HumanTask
        AssignTo := "Manager", Form := "ApprovalForm"
        On Approved  Goto Pick
        On Rejected  Goto Cancel

    Step Pick As ServerOnly
        ... pick stock ...
        Goto Ship

    Step Ship As ServerOnly
        ... call carrier ...
        Goto Done

    Step Done   As Final
    Step Cancel As Final
End Process"""
    ),
    P("The runtime persists the process state (which step, local variables, timers) in "
      "the DB after every step, so workflows survive restarts and run for days/months. "
      "<b>This is the feature that distinguishes an ERP platform from a programming "
      "language.</b>"),

    H2("Forms bound to entities"),
    P("The form designer needs a DataSource property at form level:"),
    Code(
"""<Form id="CustomerEdit" dataSource="Customer">
  <Widgets>
    <Widget type="TextBox" name="txtName"  binding="Name"/>
    <Widget type="TextBox" name="txtEmail" binding="Email"/>
    <Widget type="Button"  name="btnSave"/>
  </Widgets>
</Form>"""
    ),
    Code(
"""Sub btnSave_Click()
    Form.Save()    ' validates + persists the bound entity
End Sub"""
    ),
    P("Two-way binding, validation drawn from the entity schema, dirty-tracking — all "
      "generated by the compiler from the binding attribute."),

    H2("Security"),
    Code(
"""[Activity(RequiresPermission := "Invoice.Void")]
Public Sub VoidInvoice(invId As Long)
    ...
End Sub"""
    ),
    P("Permission strings (\"Invoice.Void\") are first-class identifiers — Nexor "
      "Command UI lists them automatically and lets the admin assign to roles."),

    H2("Reports"),
    Code(
"""Report MonthlySales
    Param StartDate As Date
    Param EndDate   As Date

    Source = From s In Sales
             Where s.Date >= StartDate And s.Date < EndDate

    Group By s.Region
    Sum     s.Total

    Layout := "MonthlySales.frx"
End Report"""
    ),
    P("The .frx is just another form, designed visually, but tagged as a report layout "
      "(paginates, no event handlers)."),
]

# ── 6. File formats
story += [
    PageBreak(),
    H1("6 · File formats / module system"),
    TableBlock([
        ["Extension", "Source / Compiled — Contains"],
        [".pro",      "Project — Atomic Activities, Process Activities, Sheets, Reports, Resources"],
        [".aba",      "Activity (source) — Sub Main, helper Subs, globals, [Activity] annotations"],
        [".frm",      "Form (source) — Widgets XML + <Code> event handlers"],
        [".sht",      "Sheet (source) — Entity schema"],
        [".prc",      "Process (source) — Workflow definition"],
        [".rpt",      "Report (source) — Query + layout"],
        [".abx / .frx / .shx / .prx / .rpx",
                      "Compiled — Bytecode + symbols"],
        [".nexor",    "Package — All compiled artifacts + manifest, signed"],
    ], col_widths=[55*mm, 110*mm]),
    Spacer8(),
    P("The package manifest lists exported activities, required permissions, schema "
      "migrations, and dependencies on other packages — so customers can install, "
      "version, and uninstall packages cleanly."),
]

# ── 7. Standard library
story += [
    H1("7 · Standard library"),
    P("Three layers, shipped as built-in packages:"),
    TableBlock([
        ["Package",     "Provides"],
        ["nexor.lang",  "Strings, Dates, Math, Regex, Collections, Json, Xml, Crypto"],
        ["nexor.io",    "Files, HTTP client, Email, FTP, ZIP"],
        ["nexor.erp",   "Money, Tax, Currencies, Localization, Audit, Number sequences"],
        ["nexor.db",    "(Internal) entity persistence, query compiler"],
    ], col_widths=[35*mm, 130*mm]),
    Spacer8(),
    P("Anything that talks to the OS (file I/O, network) is gated by per-activity "
      "permissions — a ClientOnly activity can't write to disk unless the user has "
      "granted file access."),
]

# ── 8. Implementation order
story += [
    H1("8 · Implementation order"),
    P("Build from the inside out so each layer unlocks user-visible value:"),
    Bullet("1. <b>Lexer + parser + AST</b> — produce .abx ASTs in memory."),
    Bullet("2. <b>Tree-walking interpreter</b> for the procedural core (Sub / Dim / "
           "If / While / For / Function / Return).  No types yet, no entities. "
           "Get Sub Main running with Print."),
    Bullet("3. <b>Form runtime + bindings</b> — connect the existing designer to the "
           "interpreter so click handlers fire.  90% there already."),
    Bullet("4. <b>Type system + semantic checker</b> — turn variants into typed locals "
           "where annotated; surface compile errors in the editor's gutter."),
    Bullet("5. <b>Bytecode VM</b> — replace the tree-walker, write a tiny stack "
           "machine, emit .abx.  ~5–10× speedup."),
    Bullet("6. <b>Sheets / entity engine</b> — first as in-memory records, then backed "
           "by SQLite, then by Nexor Core's PostgreSQL."),
    Bullet("7. <b>Query compiler</b> — compile From … Where … Select to backing SQL."),
    Bullet("8. <b>Process Activity engine</b> — persistent state machine, durable "
           "timers, human tasks."),
    Bullet("9. <b>Reports</b> — query + layout renderer."),
    Bullet("10. <b>Permission system + Nexor Command integration</b>."),
    Bullet("11. <b>Package format + deployment</b>."),
    Bullet("12. <b>Debugger</b> — breakpoints, step, watches, locals "
           "(the VM gives you this almost free)."),
    Spacer8(),
    P("The first three steps unlock 80% of the perceived \"the IDE works\" feeling. "
      "The middle steps (4–7) are the actual ERP differentiator.  The tail (8–12) is "
      "what turns it into a platform you can sell."),
]

# ── 9. Next deliverable
story += [
    H1("9 · A concrete next-week deliverable"),
    NoteBox(
"""<b>Pick one thing to start: steps 1+2 — lexer / parser / tree-walking
interpreter for the procedural core.</b>"""
    ),
    P("That gives you:"),
    Bullet("Click <b>Run</b> on an activity and see Print output in the OUTPUT pane."),
    Bullet("Form double-click on a button actually fires btnX_Click and runs the user's code."),
    Bullet("Compile errors land in 1 ISSUES with file/line numbers."),
    Spacer8(),
    P("Everything else is incremental on top of that foundation — and you'll learn "
      "enough about your users' actual needs from running real activities to make "
      "better decisions about the harder layers (entities, workflow, packages)."),
]


# ─── Build PDF ───────────────────────────────────────────────────────────────
def build():
    out = Path.home() / "Desktop" / "Nexor-Language-Architecture.pdf"
    out.parent.mkdir(parents=True, exist_ok=True)
    doc = SimpleDocTemplate(
        str(out),
        pagesize=A4,
        leftMargin=20*mm, rightMargin=20*mm,
        topMargin=18*mm,  bottomMargin=18*mm,
        title="Nexor — Language System Architecture",
        author="Nexor / Claude",
    )

    def footer(canvas, _doc):
        canvas.saveState()
        canvas.setFont("Helvetica", 8)
        canvas.setFillColor(colors.HexColor("#888888"))
        canvas.drawString(20*mm, 12*mm, "Nexor — Language System Architecture")
        canvas.drawRightString(A4[0] - 20*mm, 12*mm,
                               f"Page {canvas.getPageNumber()}")
        canvas.restoreState()

    doc.build(story, onFirstPage=footer, onLaterPages=footer)
    print(f"Wrote: {out}")


if __name__ == "__main__":
    build()
