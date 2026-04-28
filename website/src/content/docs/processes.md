---
title: 'Processes & BPMN'
description: 'BPMN 2.0 designer, step types, persistent workflows.'
---

A **Process Activity** is a stateful workflow modelled as a directed graph of
steps. Studio's BPMN 2.0 designer is the authoring surface; the runtime
engine executes the same graph either synchronously (Studio / Flux) or
suspendably with persistence (Core).

The on-disk format is plain BPMN 2.0 XML (`.bpmn`) — every conforming tool
(Sparx Enterprise Architect, Camunda Modeler, bpmn.io, Bizagi, Signavio) can
open and edit a Nexor process file.

## 1. Step types

| BPMN element            | Nexor name | What it does |
|-------------------------|-----------|--------------|
| `bpmn:scriptTask`       | **Server** | runs the embedded Nexor body, advances to `next` |
| `bpmn:exclusiveGateway` | **Choice** | runs the body as a `Function`; the value it `Return`s names the next step |
| `bpmn:userTask`         | **HumanTask** | opens a form (the `formRef` extension); on `Form.Accept()` runs the body, advances to `next`; on `Form.Reject()` aborts |
| `bpmn:endEvent`         | **Final**  | terminates the process |
| `bpmn:startEvent`       | (synthesised) | every process gets one auto-generated; flows into the first step |

## 2. A walk-through example

```xml
<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:nexor="http://nexor.io/bpmn/v1"
                  id="Definitions_1" targetNamespace="http://nexor.io/bpmn/v1">
  <bpmn:process id="OrderApproval" name="Order Approval" isExecutable="true">

    <bpmn:startEvent id="_StartEvent"/>

    <bpmn:scriptTask id="Validate" name="Validate Order" scriptFormat="nexor">
      <bpmn:script>
        Dim ord = Order.Find(Vars.OrderId)
        Vars.Total    = ord.Total
        Vars.IsBig    = ord.Total > 1000
      </bpmn:script>
    </bpmn:scriptTask>

    <bpmn:exclusiveGateway id="LimitCheck" name="Big enough?"/>

    <bpmn:userTask id="ManagerApprove" name="Manager approves">
      <bpmn:extensionElements>
        <nexor:formRef>ApproveOrder.frm</nexor:formRef>
      </bpmn:extensionElements>
    </bpmn:userTask>

    <bpmn:scriptTask id="AutoApprove" name="Auto-approve" scriptFormat="nexor">
      <bpmn:script>
        Vars.Approved = True
        Vars.Approver = "system"
      </bpmn:script>
    </bpmn:scriptTask>

    <bpmn:scriptTask id="Settle" name="Settle" scriptFormat="nexor">
      <bpmn:script>
        Print "approved=" & Vars.Approved & " by " & Vars.Approver
      </bpmn:script>
    </bpmn:scriptTask>

    <bpmn:endEvent id="End"/>

    <bpmn:sequenceFlow id="f1" sourceRef="_StartEvent"   targetRef="Validate"/>
    <bpmn:sequenceFlow id="f2" sourceRef="Validate"      targetRef="LimitCheck"/>
    <bpmn:sequenceFlow id="f3" sourceRef="LimitCheck"    targetRef="ManagerApprove" name="Big">
      <bpmn:conditionExpression>${returnValue == "Big"}</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="f4" sourceRef="LimitCheck"    targetRef="AutoApprove" name="Small">
      <bpmn:conditionExpression>${returnValue == "Small"}</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="f5" sourceRef="ManagerApprove" targetRef="Settle"/>
    <bpmn:sequenceFlow id="f6" sourceRef="AutoApprove"    targetRef="Settle"/>
    <bpmn:sequenceFlow id="f7" sourceRef="Settle"         targetRef="End"/>

  </bpmn:process>
</bpmn:definitions>
```

Where the **Choice** body lives — inline:

```vbnet
Function __body()         ' drawn here for explanatory purposes;
    If Vars.IsBig Then    ' inside the BPMN this is just <bpmn:script>
        Return "Big"
    Else
        Return "Small"
    End If
End Function
```

The engine evaluates the choice body, takes its return value, and matches
against the outgoing flows' `conditionExpression`s.

## 3. Authoring in Studio

```
┌─ palette ─┬───────────── canvas ─────────────┬─ inspector ─┐
│ ● Server  │   ┌───────┐    ┌─────────┐       │ Step:       │
│ ◇ Choice  │   │ Start │───▶│ Validate│       │   Approve   │
│ ▢ Human   │   └───────┘    └────┬────┘       │ Type:       │
│ ■ Final   │                      ▼            │  HumanTask  │
│           │              ┌────◇──────◇────┐  │ Form:       │
│ [+ Add ]  │   "Small" / "Big" branches    │  │  Approve.frm│
│           │      ▼              ▼         │  │             │
│           │   AutoApprove    ManagerApprove│  │ Code:       │
│           │      └─────┬───────┘          │  │ ┌─────────┐ │
│           │            ▼                  │  │ │ If …    │ │
│           │          Settle ───▶ End      │  │ └─────────┘ │
└───────────┴───────────────────────────────┴──┴─────────────┘
```

Right-click on the canvas to add steps; drag a step's edge to connect it to
another. Click a step to load its body into the inspector's code editor.
Toggle between **Diagram** and **Table** views with the header combo.

The Studio designer round-trips with EA: **Build → Export .bpmn…** writes a
file you can open in EA, edit visually, save, and **Import .bpmn…** back.

## 4. The Vars dictionary

`Vars` is a per-instance scratch space:

```vbnet
' Server step body
Vars.Total    = 250
Vars.Approved = False
Print "set total"

' Later in another step
If Vars.Approved Then ...
```

- Unknown keys read as `Empty`.
- Typed values (`Long`, `Double`, `String`, `Boolean`) round-trip through
  JSON when the engine suspends for a HumanTask. Lists and Objects do not
  round-trip — keep `Vars` flat.
- Vars are scoped to one process **instance** — different runs of the same
  process see independent bags.

## 5. The HumanTask cycle

```
        ┌─────────────┐ start             ┌──────────────┐
        │ Validate    │ ───────────────▶  │ ManagerApp.  │  pause
        │ (Server)    │                   │ (HumanTask)  │ ──────┐
        └─────────────┘                   └──────────────┘       │
                                                                 ▼
   client opens form  ◀─────────────  Core stores instance       │
        in Flux                       in process_instances       │
                                                                 │
   user fills + Form.Accept() ────▶  POST /processes/:id/resume  │
                                          ▼                       │
                                     Vars merged ─────────────────┘
                                          │
                                          ▼
                                     Server step ▶ End
```

Concretely:

1. Client kicks off:  
   `POST /api/v1/processes/Sales/OrderApproval/start  body={vars:{...}}`
2. Engine runs Validate, hits ManagerApprove (HumanTask), persists state, returns `{instance:1, status:"awaiting_human", awaiting_form:"ApproveOrder.frm", ...}`.
3. Flux opens the form. The user fills it and calls `Form.Save()` then `Form.Accept()`.
4. Client posts:  
   `POST /api/v1/processes/1/resume  body={vars:{Approver:"Alice"}}`
5. Engine runs the HumanTask body server-side, advances to `Settle`, runs to `End`. Status flips to `completed`.

See [Server-side activities](server-side.md) for the wiring code.

## 6. Engine semantics — the synchronous version

Studio and Flux ship the synchronous engine (`ProcessEngine::runProcess`) for
the **Run Process** button. HumanTask steps open the form **modally** in-place;
on `Form.Accept()` the engine resumes inline. There is no persistence — the
state lives in memory only.

This is what `▶ Run Process` does in Studio's BPMN designer and the
ProcessEditor.

## 7. Engine semantics — the persistent version

Core ships the headless engine (`ProcessEngine::runHeadless`) that suspends
on every HumanTask and returns control to the caller. Each suspend persists
the var bag + awaiting step + awaiting form id into the SQLite
`process_instances` table:

```sql
CREATE TABLE process_instances (
  instance      INTEGER PRIMARY KEY AUTOINCREMENT,
  package_id    TEXT NOT NULL,
  process_id    TEXT NOT NULL,
  status        TEXT NOT NULL,    -- pending|awaiting_human|completed|failed
  awaiting_step TEXT,
  awaiting_form TEXT,
  vars_json     TEXT NOT NULL,
  last_error    TEXT,
  started_at    TEXT NOT NULL,
  updated_at    TEXT NOT NULL
);
```

Every suspend is durable — Core can crash and restart, the instance row
persists, the user resumes whenever they get to the form.

## 8. Best practices

- **Keep Server step bodies short.** A long-running computation blocks the
  caller; the engine doesn't yet thread server-side work.
- **Validate at every Server step.** Check `Vars` for the inputs you expect;
  fail fast with a clear `Print` so the OUTPUT pane / Core's response shows
  what went wrong.
- **Don't reach across instances.** `Vars` is per-instance. To share state
  between runs, write to a Sheet.
- **Forms first, code second.** A HumanTask's form is what the user sees;
  the body code runs *after* `Form.Accept()` and should mostly transcribe
  widget values into Vars or into entities.
- **Choice steps return strings.** Even when the branches are numeric, treat
  the return as a label string so the BPMN `conditionExpression` is readable
  (`${returnValue == "Big"}`).

## 9. Limitations (today)

- Choice / Server / HumanTask / Final only — Phase 17 will add `serviceTask`,
  parallel gateways, intermediate timer events.
- WebSocket push (`workflow-step` notifications) not yet shipped — clients
  poll `GET /api/v1/processes?status=awaiting_human`.
- No retries / compensation / sub-processes yet — pure forward flow.

## 10. Where to read next

- **[Forms Guide](forms.md)** — author the form a HumanTask opens.
- **[Server-side Activities](server-side.md)** — kick off processes from client code via `<ProcessName>.Start()`.
- **[API Reference](api-reference.md)** — process REST endpoints in detail.
- **Sample:** [`samples/order-approval/`](samples/order-approval/) — a complete project with a Sheet, a Form, and a 5-step BPMN process.
