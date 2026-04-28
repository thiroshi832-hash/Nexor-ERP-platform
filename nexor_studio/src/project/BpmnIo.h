// =============================================================================
// BpmnIo — read / write BPMN 2.0 XML (OMG formal/2011-01-03) for Process
// activities.  The canonical disk format for processes is now .bpmn; Studio,
// EA, Camunda Modeler, bpmn.io, Bizagi, Signavio, and any other conforming
// tool can open the same file.
//
// Element mapping (Nexor → BPMN 2.0):
//
//     Server step       →  bpmn:scriptTask  (scriptFormat="nexor", inline body)
//     Choice step       →  bpmn:exclusiveGateway + conditional sequenceFlows
//                          (one outgoing flow per Branch, with the Return
//                           value carried in <bpmn:conditionExpression>)
//     HumanTask step    →  bpmn:userTask    (formRef in extensionElements)
//     Final step        →  bpmn:endEvent
//     (implicit Start)  →  bpmn:startEvent
//
// Nexor-specific data lives in <bpmn:extensionElements xmlns:nexor="…">,
// the OMG-blessed mechanism for vendor extensions.  EA, etc. preserve
// these elements verbatim on round-trip — the diagram stays editable in
// any BPMN tool while the runtime data stays attached.
//
// The writer also emits a <bpmndi:BPMNDiagram> with shape bounds + edge
// waypoints so EA renders the diagram exactly the way Studio drew it.
// =============================================================================
#ifndef NEXOR_STUDIO_BPMNIO_H
#define NEXOR_STUDIO_BPMNIO_H

#include <QString>
#include <QByteArray>
#include "Process.h"

namespace nx {

class BpmnIo {
public:
    // Decode bytes that start with <?xml … ?> and a <bpmn:definitions> root.
    // Populates `outProcess` (meta, steps, layout) and returns true on
    // success.  On failure the caller can inspect *errorOut.
    static bool readBpmn(const QByteArray &xml,
                         Process &outProcess,
                         QString *errorOut = nullptr);

    // Encode `process` as a single bpmn:definitions document, including the
    // BPMNDiagram.  Always succeeds (returns the bytes); validation is the
    // caller's job (or Phase 12f's overlay).
    static QByteArray writeBpmn(const Process &process);

    // Returns true if `bytes` starts with a recognisable BPMN 2.0 root.
    static bool sniff(const QByteArray &bytes);
};

} // namespace nx

#endif // NEXOR_STUDIO_BPMNIO_H
