// Tiny harness that exercises BpmnIo::readBpmn + writeBpmn end-to-end.
// Reads a .bpmn from argv[1], parses it through Process, writes it back to
// argv[2] (or stdout), and prints a summary of the parsed steps so the
// caller can eyeball the round-trip.
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include "project/Process.h"
#include "project/BpmnIo.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        qInfo() << "usage: bpmn_roundtrip <in.bpmn> [out.bpmn]";
        return 1;
    }
    QFile inFile(argv[1]);
    if (!inFile.open(QIODevice::ReadOnly)) {
        qCritical() << "cannot read" << argv[1];
        return 2;
    }

    Process p;
    QString err;
    if (!nx::BpmnIo::readBpmn(inFile.readAll(), p, &err)) {
        qCritical().noquote() << "parse error:" << err;
        return 3;
    }
    qInfo().noquote() << QString("Parsed %1 v%2: %3 step(s)")
                          .arg(p.meta().id, p.meta().title)
                          .arg(p.steps().size());
    for (const StepSpec &s : p.steps()) {
        QString line = QString("  %1  type=%2  next=%3  form=%4  branches=%5")
                         .arg(s.id, -20).arg(s.type, -10)
                         .arg(s.nextId, -16).arg(s.formId, -12)
                         .arg(s.branches.size());
        qInfo().noquote() << line;
        for (const auto &b : s.branches) {
            qInfo().noquote() << QString("       → %1   target=%2")
                                  .arg(b.returnValue, -10).arg(b.targetId);
        }
    }

    QByteArray out = nx::BpmnIo::writeBpmn(p);
    if (argc >= 3) {
        QFile o(argv[2]);
        if (!o.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCritical() << "cannot write" << argv[2];
            return 4;
        }
        o.write(out);
    } else {
        QTextStream(stdout) << QString::fromUtf8(out);
    }

    // Re-parse the output to confirm we didn't break the wire shape.
    Process p2;
    if (!nx::BpmnIo::readBpmn(out, p2, &err)) {
        qCritical().noquote() << "round-trip parse error:" << err;
        return 5;
    }
    qInfo().noquote() << QString("Round-trip OK: %1 step(s) on second parse.")
                          .arg(p2.steps().size());
    return 0;
}
