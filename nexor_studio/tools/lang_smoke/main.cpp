// Language smoke test.  Each entry is { name, source, expectedPrint }.
// expectedPrint is either:
//   - a string: must match the captured Print output exactly
//   - a string starting with "ERR:": expect a compile-time or runtime
//     failure that contains that substring after "ERR:"
//   - "" (empty): just expect compilation + execution to succeed
//
// Reports PASS / FAIL per case; exit code = number of failures.
#include <QCoreApplication>
#include <QTextStream>
#include <QStringList>
#include <QDir>
#include <QFile>
#include "language/NexorRuntime.h"
#include "language/EntityStore.h"

struct Case {
    const char *name;
    const char *src;
    const char *expect;
};

static const Case cases[] = {

    // ── Basics ─────────────────────────────────────────────────────────
    { "print-string",      "Sub Test()\n  Print \"hi\"\nEnd Sub",                    "hi" },
    { "print-int",         "Sub Test()\n  Print 42\nEnd Sub",                          "42" },
    { "print-double",      "Sub Test()\n  Print 3.14\nEnd Sub",                        "3.14" },
    { "print-bool",        "Sub Test()\n  Print True\nEnd Sub",                        "True" },

    // ── Operators ──────────────────────────────────────────────────────
    { "add",               "Sub Test()\n  Print 2+3\nEnd Sub",                          "5" },
    { "sub",               "Sub Test()\n  Print 10-7\nEnd Sub",                         "3" },
    { "mul",               "Sub Test()\n  Print 4*5\nEnd Sub",                          "20" },
    { "div-int",           "Sub Test()\n  Print 10/4\nEnd Sub",                         "2.5" },
    { "mod",               "Sub Test()\n  Print 10 Mod 3\nEnd Sub",                     "1" },
    { "concat-amp",        "Sub Test()\n  Print \"a\" & \"b\"\nEnd Sub",                "ab" },
    { "concat-mixed",      "Sub Test()\n  Print \"x=\" & 5\nEnd Sub",                   "x=5" },
    { "precedence",        "Sub Test()\n  Print 1+2*3\nEnd Sub",                        "7" },
    { "parens",            "Sub Test()\n  Print (1+2)*3\nEnd Sub",                      "9" },
    { "negative",          "Sub Test()\n  Dim x = -5\n  Print x\nEnd Sub",              "-5" },
    { "compare-eq-true",   "Sub Test()\n  Print 1=1\nEnd Sub",                          "True" },
    { "compare-neq",       "Sub Test()\n  Print 1<>2\nEnd Sub",                         "True" },
    { "logical-and",       "Sub Test()\n  Print True And False\nEnd Sub",               "False" },
    { "logical-or",        "Sub Test()\n  Print False Or True\nEnd Sub",                "True" },
    { "not",               "Sub Test()\n  Print Not False\nEnd Sub",                    "True" },

    // ── Identity ───────────────────────────────────────────────────────
    { "is-nothing-true",   "Sub Test()\n  Dim x\n  Print x Is Nothing\nEnd Sub",        "True" },
    { "is-nothing-false",  "Sub Test()\n  Dim x = 5\n  Print x Is Nothing\nEnd Sub",    "False" },
    { "isnot-nothing",     "Sub Test()\n  Dim x = 5\n  Print x IsNot Nothing\nEnd Sub", "True" },

    // ── Control flow ───────────────────────────────────────────────────
    { "if-multi-then",     "Sub Test()\n  If 1=1 Then\n    Print \"ok\"\n  End If\nEnd Sub", "ok" },
    { "if-else",           "Sub Test()\n  If 1=2 Then\n    Print \"x\"\n  Else\n    Print \"ok\"\n  End If\nEnd Sub", "ok" },
    { "if-elseif",         "Sub Test()\n  If 1=2 Then\n    Print \"a\"\n  ElseIf 1=1 Then\n    Print \"ok\"\n  End If\nEnd Sub", "ok" },
    { "if-single-line",    "Sub Test()\n  If 1=1 Then Print \"ok\"\nEnd Sub",           "ok" },
    { "if-single-with-else","Sub Test()\n  If 1=2 Then Print \"x\" Else Print \"ok\"\nEnd Sub", "ok" },
    { "while",             "Sub Test()\n  Dim i = 0\n  While i < 3\n    i = i + 1\n  Wend\n  Print i\nEnd Sub", "3" },
    { "for-to",            "Sub Test()\n  Dim sum = 0\n  For i = 1 To 5\n    sum = sum + i\n  Next i\n  Print sum\nEnd Sub", "15" },
    { "for-step",          "Sub Test()\n  Dim s = \"\"\n  For i = 1 To 9 Step 2\n    s = s & i\n  Next i\n  Print s\nEnd Sub", "13579" },

    // ── Subs / Functions ──────────────────────────────────────────────
    { "function-return",   "Function Greet(n)\n  Return \"hi \" & n\nEnd Function\nSub Test()\n  Print Greet(\"world\")\nEnd Sub", "hi world" },
    { "exit-sub",          "Sub Test()\n  Print \"a\"\n  Exit Sub\n  Print \"b\"\nEnd Sub", "a" },
    { "func-default-empty","Function F()\n  Dim x\nEnd Function\nSub Test()\n  Print F() Is Nothing\nEnd Sub", "True" },

    // ── Built-ins ──────────────────────────────────────────────────────
    { "len",               "Sub Test()\n  Print Len(\"nexor\")\nEnd Sub",               "5" },
    { "ucase",             "Sub Test()\n  Print UCase(\"abc\")\nEnd Sub",               "ABC" },
    { "lcase",             "Sub Test()\n  Print LCase(\"ABC\")\nEnd Sub",               "abc" },
    { "trim",              "Sub Test()\n  Print Trim(\"  x  \")\nEnd Sub",              "x" },
    { "left",              "Sub Test()\n  Print Left(\"nexor\", 3)\nEnd Sub",           "nex" },
    { "right",             "Sub Test()\n  Print Right(\"nexor\", 3)\nEnd Sub",          "xor" },
    { "mid",               "Sub Test()\n  Print Mid(\"nexor\", 2, 3)\nEnd Sub",         "exo" },
    { "abs",               "Sub Test()\n  Print Abs(-7)\nEnd Sub",                      "7" },
    { "int",               "Sub Test()\n  Print Int(3.9)\nEnd Sub",                     "3" },
    { "round",             "Sub Test()\n  Print Round(3.456, 2)\nEnd Sub",              "3.46" },
    { "cstr",              "Sub Test()\n  Print CStr(123)\nEnd Sub",                    "123" },
    { "cint",              "Sub Test()\n  Print CInt(\"42\")\nEnd Sub",                 "42" },
    { "cdbl",              "Sub Test()\n  Print CDbl(\"3.14\")\nEnd Sub",               "3.14" },
    { "cbool",             "Sub Test()\n  Print CBool(\"true\")\nEnd Sub",              "True" },

    // ── No-paren statement-form sub call (VBScript style) ─────────────
    // We can't see MsgBox dialogs from a console test, but we can prove
    // the parser routes the args through to a builtin: define a Sub that
    // captures via Print, then call it both ways.
    { "noparen-call-one",  "Sub Say(s)\n  Print s\nEnd Sub\nSub Test()\n  Say \"hi\"\nEnd Sub", "hi" },
    { "noparen-call-many", "Sub Say(a, b)\n  Print a & \"-\" & b\nEnd Sub\nSub Test()\n  Say \"x\", \"y\"\nEnd Sub", "x-y" },
    { "noparen-call-ident","Sub Say(s)\n  Print s\nEnd Sub\nSub Test()\n  Dim m = \"hello\"\n  Say m\nEnd Sub", "hello" },
    { "paren-call-still-works","Sub Say(s)\n  Print s\nEnd Sub\nSub Test()\n  Say(\"hi\")\nEnd Sub", "hi" },

    // ── Vars (process bag) — should silently no-op on a fresh interp ──
    { "vars-no-store",     "Sub Test()\n  Vars.X = 5\n  Print Vars.X Is Nothing\nEnd Sub", "True" },

    // ── Annotations ────────────────────────────────────────────────────
    { "annotation-runs",   "[Activity(RunsOn := Either)]\nSub Test()\n  Print \"ok\"\nEnd Sub", "ok" },

    // ── Comments ───────────────────────────────────────────────────────
    { "comment-line",      "Sub Test()\n  ' this is a comment\n  Print \"ok\"\nEnd Sub", "ok" },

    // ── Error paths ────────────────────────────────────────────────────
    { "compile-bad-syntax","Sub Test()\n  If True Then\n  Print x\n",                  "ERR:expected" },
    { "div-by-zero",       "Sub Test()\n  Print 1/0\nEnd Sub",                          "ERR:" },
    { "missing-end",       "Sub Test()\n  Print \"x\"\n",                               "ERR:" },
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    int failed = 0;
    int total  = sizeof(cases) / sizeof(cases[0]);
    for (const Case &c : cases) {
        nx::NexorRuntime rt;
        QString captured;
        QString errMsg;
        rt.setOutput([&captured](const QString &line){
            if (!captured.isEmpty()) captured += '\n';
            captured += line;
        });
        rt.setError([&errMsg](const QString &er){
            if (errMsg.isEmpty()) errMsg = er;
        });

        QString expect = QString::fromUtf8(c.expect);
        bool wantErr = expect.startsWith("ERR:");
        QString errSubstr = wantErr ? expect.mid(4) : QString();

        bool compiled = rt.compile(QString::fromUtf8(c.src), c.name);
        if (compiled) rt.call("Test");

        bool gotErr = !compiled || !errMsg.isEmpty();
        QString actualErr = compiled ? errMsg : rt.lastError();
        bool ok;
        if (wantErr) {
            ok = gotErr && (errSubstr.isEmpty() ||
                            actualErr.contains(errSubstr, Qt::CaseInsensitive));
        } else if (expect.isEmpty()) {
            ok = !gotErr;
        } else {
            ok = !gotErr && captured == expect;
        }

        if (ok) {
            // out << QString("  pass  %1\n").arg(c.name);
        } else {
            ++failed;
            QString got = wantErr ? actualErr : captured;
            out << QString("  FAIL  %1\n").arg(c.name);
            out << QString("        want : %1\n").arg(expect);
            out << QString("        got  : %1\n").arg(got);
        }
    }
    // ── EntityStore error-sink coverage ────────────────────────────────
    // Save against an unopened store must route a user-visible message to
    // the registered sink (and still return false), instead of failing
    // silently via qWarning.  Mirrors the no-project-loaded path that
    // Studio hits when an .aba is opened without its .nxproj.
    {
        ++total;
        nx::EntityStore store;
        QString captured;
        store.setErrorSink([&captured](const QString &m){ captured = m; });

        nx::SheetSchema schema;
        schema.sheetId = "Customer";
        nx::SheetSchemaField idF;   idF.name = "Id";   idF.type = "Long";   idF.isKey = true;
        nx::SheetSchemaField nameF; nameF.name = "Name"; nameF.type = "String";
        schema.fields << idF << nameF;
        store.registerSheet(schema);

        auto *t = store.table("Customer");
        bool saveOk = t && t->save(t->create());
        bool ok = !saveOk
                  && captured.contains("EntityStore", Qt::CaseInsensitive)
                  && captured.contains("Customer");
        if (!ok) {
            ++failed;
            out << "  FAIL  entitystore-save-unopened\n";
            out << QString("        want : Save==false, sink mentions EntityStore + Customer\n");
            out << QString("        got  : saveOk=%1, sink=%2\n")
                    .arg(saveOk ? "true" : "false", captured);
        }
    }

    // ── Entity member-assignment round-trip ────────────────────────────
    // Reproduces SeedDemoData's path: open store, register Customer
    // schema, then via interpreter do  Dim c = Customer.New() :
    // c.Name = "Acme" : c.Save() — and assert the row landed with Name
    // set.  Surfaces any breakage in MemberAssign → Entity::set routing.
    {
        ++total;
        nx::NexorRuntime rt;
        QString captured, errMsg;
        rt.setOutput([&captured](const QString &line){
            if (!captured.isEmpty()) captured += '\n';
            captured += line;
        });
        rt.setError([&errMsg](const QString &er){
            if (!errMsg.isEmpty()) errMsg += '\n';
            errMsg += er;
        });

        QString tmpDb = QDir::tempPath() + "/nx_lang_smoke_entity.ndb";
        QFile::remove(tmpDb);
        rt.interpreter()->entityStore()->open(tmpDb);

        nx::SheetSchema cust;
        cust.sheetId = "Customer";
        nx::SheetSchemaField idF;   idF.name = "Id";   idF.type = "Long";   idF.isKey = true; idF.required = true;
        nx::SheetSchemaField nameF; nameF.name = "Name"; nameF.type = "String"; nameF.required = true;
        cust.fields << idF << nameF;
        rt.interpreter()->registerSheet(cust);

        const char *src =
            "Sub Test()\n"
            "  Dim c = Customer.New()\n"
            "  c.Name = \"Acme\"\n"
            "  c.Save()\n"
            "  Print Customer.Count() & \"|\" & Customer.Find(1).Name\n"
            "End Sub";
        bool compiled = rt.compile(src, "entity-roundtrip");
        if (compiled) rt.call("Test");

        bool ok = compiled && errMsg.isEmpty() && captured == "1|Acme";
        if (!ok) {
            ++failed;
            out << "  FAIL  entity-member-assignment-roundtrip\n";
            out << QString("        want : 1|Acme, no errors\n");
            out << QString("        got  : print=%1 err=%2\n").arg(captured, errMsg);
        }
        QFile::remove(tmpDb);
    }

    out << QString("\n%1 / %2 cases passed (%3 failed)\n")
            .arg(total - failed).arg(total).arg(failed);
    return failed;
}
