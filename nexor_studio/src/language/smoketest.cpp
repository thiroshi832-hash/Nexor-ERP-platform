// Manual smoke test for the language pipeline — compiles a small Nexor script
// and runs Sub Main, printing to stdout.  Built only when SMOKE is defined.
#ifdef NEXOR_LANG_SMOKE

#include "NexorRuntime.h"
#include <QCoreApplication>
#include <QTextStream>
#include <iostream>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    nx::NexorRuntime rt;

    rt.setOutput([](const QString &line){
        std::cout << line.toStdString() << "\n";
    });
    rt.setError([](const QString &err){
        std::cerr << "ERROR: " << err.toStdString() << "\n";
    });

    QString src = R"NEXOR(
        ' Smoke test
        Dim total
        total = 0
        Sub Main()
            Print "Hello, Nexor!"
            For i = 1 To 5
                total = total + i
                Print "i=" & CStr(i) & "  total=" & CStr(total)
            Next
            If total = 15 Then
                Print "Sum 1..5 ok: " & CStr(total)
            Else
                Print "WRONG: total=" & CStr(total)
            End If
            Dim s
            s = "abc"
            Print UCase(s) & " has length " & CStr(Len(s))
        End Sub
    )NEXOR";

    if (!rt.compile(src, "smoke")) {
        std::cerr << "Compile failed: " << rt.lastError().toStdString() << "\n";
        return 1;
    }
    rt.call("Main");
    return 0;
}

#endif // NEXOR_LANG_SMOKE
