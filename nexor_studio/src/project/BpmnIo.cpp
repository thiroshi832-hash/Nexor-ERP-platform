#include "BpmnIo.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QHash>
#include <QPointF>
#include <QSizeF>

namespace nx {

namespace {

constexpr const char *NS_BPMN  = "http://www.omg.org/spec/BPMN/20100524/MODEL";
constexpr const char *NS_DI    = "http://www.omg.org/spec/BPMN/20100524/DI";
constexpr const char *NS_DC    = "http://www.omg.org/spec/DD/20100524/DC";
constexpr const char *NS_DI2   = "http://www.omg.org/spec/DD/20100524/DI";
constexpr const char *NS_NEXOR = "http://nexor.io/bpmn/v1";

QSizeF defaultSizeFor(const QString &type) {
    if (type.compare("Choice", Qt::CaseInsensitive) == 0)    return {50, 50};   // diamond
    if (type.compare("Final",  Qt::CaseInsensitive) == 0)    return {36, 36};   // circle
    return {100, 60};                                                            // task
}

// --- Reader -------------------------------------------------------------

// Minimal interim model captured during the first pass.
struct Element {
    QString id;
    QString name;
    QString tag;       // "scriptTask" / "userTask" / "exclusiveGateway" / "endEvent" / "startEvent"
    QString script;    // for scriptTask
    QString formRef;   // from extensionElements (Nexor)
};
struct Flow {
    QString id, sourceRef, targetRef, name, condition;
};
struct Shape {
    QString refId;
    QPointF pos;
    QSizeF  size;
};

QString stepTypeForTag(const QString &tag) {
    if (tag == "scriptTask")        return "Server";
    if (tag == "userTask")          return "HumanTask";
    if (tag == "exclusiveGateway")  return "Choice";
    if (tag == "endEvent")          return "Final";
    if (tag == "startEvent")        return "Start";       // synthesised
    return "Server";
}

void readExtensionElements(QXmlStreamReader &r, Element &e) {
    while (!r.atEnd()) {
        r.readNext();
        if (r.isEndElement() && r.name() == "extensionElements") return;
        if (!r.isStartElement()) continue;
        if (r.namespaceUri() == NS_NEXOR && r.name() == "formRef") {
            e.formRef = r.readElementText();
        } else {
            r.readElementText(QXmlStreamReader::SkipChildElements);
        }
    }
}

} // namespace

bool BpmnIo::sniff(const QByteArray &bytes) {
    QXmlStreamReader r(bytes);
    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement()) return r.name() == "definitions";
    }
    return false;
}

bool BpmnIo::readBpmn(const QByteArray &xml, Process &out, QString *errorOut) {
    QVector<Element>    elems;
    QVector<Flow>       flows;
    QHash<QString, Shape> shapes;
    QHash<QString, QString> nameById;

    QXmlStreamReader r(xml);

    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement()) continue;
        const auto n = r.name();
        const auto attrs = r.attributes();

        if (n == "definitions") continue;

        if (n == "process") {
            ProcessMeta m = out.meta();
            QString id = attrs.value("id").toString();
            QString name = attrs.value("name").toString();
            if (m.id.isEmpty())    m.id    = id;
            if (m.title.isEmpty()) m.title = name.isEmpty() ? id : name;
            out.setMeta(m);
            continue;
        }

        if (n == "scriptTask"       || n == "userTask"  ||
            n == "exclusiveGateway" || n == "endEvent"  ||
            n == "startEvent") {
            Element e;
            e.tag  = n.toString();
            e.id   = attrs.value("id").toString();
            e.name = attrs.value("name").toString();
            nameById.insert(e.id, e.name);

            // Walk the element body to pick up <script>, <extensionElements>.
            while (!r.atEnd()) {
                r.readNext();
                if (r.isEndElement() && r.name() == n) break;
                if (!r.isStartElement()) continue;
                if (r.name() == "script")            e.script = r.readElementText();
                else if (r.name() == "extensionElements") readExtensionElements(r, e);
                else r.readElementText(QXmlStreamReader::SkipChildElements);
            }
            elems.append(e);
            continue;
        }

        if (n == "sequenceFlow") {
            Flow f;
            f.id        = attrs.value("id").toString();
            f.sourceRef = attrs.value("sourceRef").toString();
            f.targetRef = attrs.value("targetRef").toString();
            f.name      = attrs.value("name").toString();
            while (!r.atEnd()) {
                r.readNext();
                if (r.isEndElement() && r.name() == "sequenceFlow") break;
                if (r.isStartElement() && r.name() == "conditionExpression")
                    f.condition = r.readElementText();
            }
            flows.append(f);
            continue;
        }

        if (n == "BPMNShape") {
            Shape s;
            s.refId = attrs.value("bpmnElement").toString();
            while (!r.atEnd()) {
                r.readNext();
                if (r.isEndElement() && r.name() == "BPMNShape") break;
                if (r.isStartElement() && r.name() == "Bounds") {
                    auto a = r.attributes();
                    s.pos  = QPointF(a.value("x").toDouble(), a.value("y").toDouble());
                    s.size = QSizeF (a.value("width").toDouble(), a.value("height").toDouble());
                }
            }
            shapes.insert(s.refId, s);
        }
    }

    if (r.hasError()) {
        if (errorOut) *errorOut = r.errorString();
        return false;
    }

    // Build StepSpecs.  startEvent collapses into the "Start" position --
    // we synthesise its outgoing flow into the next step's nothing; the
    // engine's startIndex() picks "Start" by id, falling back to first step.
    QVector<StepSpec> steps;
    for (const Element &e : elems) {
        StepSpec s;
        s.id   = e.id.isEmpty() ? QString("Step%1").arg(steps.size()+1) : e.id;
        s.name = e.name;
        s.type = stepTypeForTag(e.tag);
        s.code = e.script;
        s.formId = e.formRef;
        if (shapes.contains(s.id)) {
            s.pos  = shapes.value(s.id).pos;
            s.size = shapes.value(s.id).size;
        } else {
            s.size = defaultSizeFor(s.type);
        }
        steps.append(s);
    }

    // Resolve sequence flows.
    auto findStep = [&](const QString &id) -> StepSpec* {
        for (auto &s : steps) if (s.id == id) return &s;
        return nullptr;
    };

    for (const Flow &f : flows) {
        StepSpec *src = findStep(f.sourceRef);
        if (!src) continue;
        if (src->type == "Choice") {
            StepSpec::Branch b;
            // condition body looks like: ${returnValue == "Foo"}  OR just "Foo"
            QString val = f.condition;
            int eq = val.indexOf("==");
            if (eq >= 0) val = val.mid(eq + 2);
            val.remove('"').remove('}').remove('\'').remove(' ');
            b.returnValue = val;
            b.targetId    = f.targetRef;
            b.label       = f.name.isEmpty() ? val : f.name;
            src->branches.append(b);
            if (src->nextId.isEmpty()) src->nextId = f.targetRef;   // fallback
        } else if (src->type == "Start") {
            // The synthesised start event names the entry step via its
            // outgoing flow; surface that as the canonical Start id.
            StepSpec *first = findStep(f.targetRef);
            if (first && first->id != "Start") {
                // Add a synthetic Start row pointing to the first real step
                // so engine startIndex() works.
                StepSpec startRow;
                startRow.id     = "Start";
                startRow.type   = "Server";
                startRow.nextId = f.targetRef;
                startRow.code.clear();
                startRow.size   = QSizeF(36, 36);
                startRow.pos    = src->pos;
                steps.prepend(startRow);
            }
        } else if (src->type != "Final") {
            src->nextId = f.targetRef;
        }
    }

    // Drop the BPMN startEvent placeholder (we already synthesised "Start").
    for (int i = steps.size() - 1; i >= 0; --i) {
        if (steps[i].type == "Start" && steps[i].id != "Start")
            steps.remove(i);
    }

    out.steps() = steps;
    return true;
}

// --- Writer -------------------------------------------------------------

namespace {
QString tagFor(const StepSpec &s) {
    QString t = s.type.toLower();
    if (t == "server")    return "scriptTask";
    if (t == "humantask") return "userTask";
    if (t == "choice")    return "exclusiveGateway";
    if (t == "final")     return "endEvent";
    return "scriptTask";
}
} // namespace

QByteArray BpmnIo::writeBpmn(const Process &process) {
    QByteArray buf;
    QXmlStreamWriter w(&buf);
    w.setAutoFormatting(true);

    w.writeStartDocument();
    w.writeNamespace(NS_BPMN,  "bpmn");
    w.writeNamespace(NS_DI,    "bpmndi");
    w.writeNamespace(NS_DC,    "dc");
    w.writeNamespace(NS_DI2,   "di");
    w.writeNamespace(NS_NEXOR, "nexor");

    w.writeStartElement(NS_BPMN, "definitions");
    w.writeAttribute("id",              "Definitions_" + process.meta().id);
    w.writeAttribute("targetNamespace", NS_NEXOR);

    // ---- Semantic model
    w.writeStartElement(NS_BPMN, "process");
    w.writeAttribute("id",            process.meta().id);
    w.writeAttribute("name",          process.meta().title);
    w.writeAttribute("isExecutable",  "true");

    // Always emit a startEvent that targets whichever step is the engine's
    // "Start" (or first row if not present).  This makes EA happy.
    QString firstId = process.steps().isEmpty()
                         ? QString()
                         : process.steps().first().id;
    int startIdx = process.indexOfStep("Start");
    if (startIdx >= 0) {
        const StepSpec &s = process.steps().at(startIdx);
        if (!s.nextId.isEmpty()) firstId = s.nextId;
    }
    w.writeStartElement(NS_BPMN, "startEvent");
    w.writeAttribute("id", "_StartEvent");
    w.writeEndElement();

    // Gather outgoing edges from each step.
    struct OutFlow { QString id, source, target, label, condition; };
    QVector<OutFlow> flows;
    int flowCounter = 1;
    if (!firstId.isEmpty()) {
        flows.append({QString("_Flow_%1").arg(flowCounter++),
                      "_StartEvent", firstId, QString(), QString()});
    }
    for (const StepSpec &s : process.steps()) {
        if (s.id == "Start") continue;
        if (s.type.compare("Choice", Qt::CaseInsensitive) == 0) {
            for (const auto &b : s.branches) {
                OutFlow f;
                f.id     = QString("_Flow_%1").arg(flowCounter++);
                f.source = s.id;
                f.target = b.targetId;
                f.label  = b.label;
                f.condition = QString("${returnValue == \"%1\"}").arg(b.returnValue);
                flows.append(f);
            }
            // If no branches defined but a fallback nextId exists, emit it too.
            if (s.branches.isEmpty() && !s.nextId.isEmpty()) {
                flows.append({QString("_Flow_%1").arg(flowCounter++),
                              s.id, s.nextId, QString(), QString()});
            }
        } else if (s.type.compare("Final", Qt::CaseInsensitive) != 0
                && !s.nextId.isEmpty()) {
            flows.append({QString("_Flow_%1").arg(flowCounter++),
                          s.id, s.nextId, QString(), QString()});
        }
    }

    // Emit flow nodes in original order.  All element attributes must be
    // written before any child element (otherwise QXmlStreamWriter places
    // them mid-stream as text).
    for (const StepSpec &s : process.steps()) {
        if (s.id == "Start") continue;     // synthesised separately above
        QString tag = tagFor(s);
        w.writeStartElement(NS_BPMN, tag);
        w.writeAttribute("id",   s.id);
        if (!s.name.isEmpty()) w.writeAttribute("name", s.name);
        if (tag == "scriptTask" && !s.code.isEmpty())
            w.writeAttribute("scriptFormat", "nexor");

        // Nexor-specific data in extensionElements.
        if (!s.formId.isEmpty()) {
            w.writeStartElement(NS_BPMN, "extensionElements");
            w.writeStartElement(NS_NEXOR, "formRef");
            w.writeCharacters(s.formId);
            w.writeEndElement();   // formRef
            w.writeEndElement();   // extensionElements
        }

        // Incoming / outgoing references — recommended by the spec for
        // strict validators (Bizagi cares; EA tolerates omission).
        for (const OutFlow &f : flows)
            if (f.target == s.id) {
                w.writeStartElement(NS_BPMN, "incoming");
                w.writeCharacters(f.id);
                w.writeEndElement();
            }
        for (const OutFlow &f : flows)
            if (f.source == s.id) {
                w.writeStartElement(NS_BPMN, "outgoing");
                w.writeCharacters(f.id);
                w.writeEndElement();
            }

        if (tag == "scriptTask" && !s.code.isEmpty()) {
            w.writeStartElement(NS_BPMN, "script");
            w.writeCDATA(s.code);
            w.writeEndElement();
        }
        w.writeEndElement();
    }

    // Sequence flows.
    for (const OutFlow &f : flows) {
        w.writeStartElement(NS_BPMN, "sequenceFlow");
        w.writeAttribute("id",        f.id);
        w.writeAttribute("sourceRef", f.source);
        w.writeAttribute("targetRef", f.target);
        if (!f.label.isEmpty()) w.writeAttribute("name", f.label);
        if (!f.condition.isEmpty()) {
            w.writeStartElement(NS_BPMN, "conditionExpression");
            w.writeAttribute(QStringLiteral("xmlns:xsi"),
                             QStringLiteral("http://www.w3.org/2001/XMLSchema-instance"));
            w.writeAttribute(QStringLiteral("xsi:type"), QStringLiteral("tFormalExpression"));
            w.writeCharacters(f.condition);
            w.writeEndElement();
        }
        w.writeEndElement();
    }

    w.writeEndElement();   // process

    // ---- Diagram Interchange
    w.writeStartElement(NS_DI, "BPMNDiagram");
    w.writeAttribute("id", "BPMNDiagram_1");
    w.writeStartElement(NS_DI, "BPMNPlane");
    w.writeAttribute("id",          "BPMNPlane_1");
    w.writeAttribute("bpmnElement", process.meta().id);

    // Synthesised start event needs a layout slot too.
    {
        w.writeStartElement(NS_DI, "BPMNShape");
        w.writeAttribute("id",          "_StartEvent_di");
        w.writeAttribute("bpmnElement", "_StartEvent");
        w.writeStartElement(NS_DC, "Bounds");
        w.writeAttribute("x", "60"); w.writeAttribute("y", "100");
        w.writeAttribute("width", "36"); w.writeAttribute("height", "36");
        w.writeEndElement();
        w.writeEndElement();
    }

    for (const StepSpec &s : process.steps()) {
        if (s.id == "Start") continue;
        QPointF p = s.pos;
        QSizeF  z = s.size.isValid() ? s.size : defaultSizeFor(s.type);
        if (p.isNull()) p = QPointF(160 + 140 * std::max(0, process.indexOfStep(s.id)), 100);
        w.writeStartElement(NS_DI, "BPMNShape");
        w.writeAttribute("id",          s.id + "_di");
        w.writeAttribute("bpmnElement", s.id);
        w.writeStartElement(NS_DC, "Bounds");
        w.writeAttribute("x",      QString::number(p.x()));
        w.writeAttribute("y",      QString::number(p.y()));
        w.writeAttribute("width",  QString::number(z.width()));
        w.writeAttribute("height", QString::number(z.height()));
        w.writeEndElement();
        w.writeEndElement();
    }

    for (const OutFlow &f : flows) {
        w.writeStartElement(NS_DI, "BPMNEdge");
        w.writeAttribute("id",          f.id + "_di");
        w.writeAttribute("bpmnElement", f.id);
        // Two minimal waypoints — EA can re-route on its own.
        for (int wp = 0; wp < 2; ++wp) {
            w.writeStartElement(NS_DI2, "waypoint");
            w.writeAttribute("x", "0");
            w.writeAttribute("y", "0");
            w.writeEndElement();
        }
        w.writeEndElement();
    }

    w.writeEndElement();  // BPMNPlane
    w.writeEndElement();  // BPMNDiagram

    w.writeEndElement();  // definitions
    w.writeEndDocument();
    return buf;
}

} // namespace nx
