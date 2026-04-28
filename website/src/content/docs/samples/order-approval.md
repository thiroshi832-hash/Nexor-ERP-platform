---
title: 'Sample: Order Approval'
description: 'BPMN workflow with Choice and HumanTask.'
---

A 5-step BPMN workflow with a Server step, a Choice gateway, a HumanTask,
and two terminating Final paths. Demonstrates how Studio's BPMN designer,
the synchronous engine (Studio / Flux), and Core's persistent engine all
consume the same `.bpmn` file.

## What's in here

```
order-approval/
  OrderApproval.pro                      project file
  sheets/Order/Order.sht                 entity schema
  activities/Sales/Sales.aba             activity (Sub Main + form list)
  activities/Sales/OrderEntry.frm        order-entry form (bound to Order)
  activities/Sales/Approve.frm           HumanTask approval form
  processes/Approval/Approval.bpmn       BPMN 2.0 process
```

## The process

```
   ┌─────────┐  ┌──────────┐  ┌─◇─────────────◇─┐
   │ Start   │─▶│ Validate │─▶│  LimitCheck     │
   └─────────┘  └──────────┘  └────┬────────┬───┘
                                "Big" │ "Small"
                                      ▼        ▼
                              ┌────▢───────┐ ┌────────────┐
                              │ ManagerApp │ │ AutoApprove│
                              │ (HumanTask)│ │ (Server)   │
                              └─────┬──────┘ └────┬───────┘
                                    └──────┬──────┘
                                           ▼
                                       ┌─■──┐
                                       │End │
                                       └────┘
```

| Step           | Type      | Body |
|----------------|-----------|------|
| `_StartEvent`  | start     | (synthesised) |
| `Validate`     | Server    | loads the order, sets `Vars.Total`, sets `Vars.IsBig` |
| `LimitCheck`   | Choice    | returns `"Big"` or `"Small"` based on `Vars.IsBig` |
| `ManagerApprove` | HumanTask | opens `Approve.frm`; on Accept records the approver |
| `AutoApprove`  | Server    | sets `Vars.Approver = "system"`, `Vars.Approved = True` |
| `End`          | endEvent  | terminates |

## Try it locally (synchronous engine)

1. Open `OrderApproval.pro` in Studio.
2. **Ctrl+F5** runs `Sub Main` from `Sales.aba` — it seeds a couple of orders into the local SQLite.
3. Open `processes/Approval/Approval.bpmn` from the project tree → Studio's BPMN designer renders the diagram.
4. Click **▶ Run Process** in the header bar. Studio will:
   - run `Validate` (sets Vars.Total / Vars.IsBig for the test order),
   - run the `LimitCheck` Choice (returns `"Big"` or `"Small"`),
   - if Big, open `Approve.frm` modally — fill in the approver name and click **Approve**,
   - run the post-form body, jump to `End`.

## Try it as a persistent process (Core)

1. **Ctrl+B** Build, **Ctrl+Shift+P** Publish to Core (after Command **Deploy**).
2. From any client (Flux, curl, your own integration), kick off an instance:

   ```bash
   curl -X POST http://localhost:7421/api/v1/processes/OrderApproval/Approval/start \
        -H "Authorization: Bearer $token" \
        -H "Content-Type: application/json" \
        -d '{"vars":{"OrderId":1}}'
   ```

   Response (Big order):

   ```json
   {
     "instance":1,
     "status":"awaiting_human",
     "awaiting_step":"ManagerApprove",
     "awaiting_form":"Approve.frm",
     "vars":{"OrderId":1, "Total":1500, "IsBig":true},
     "output":["validated total=1500"]
   }
   ```

3. The user opens `Approve.frm` in Flux, fills it, hits Approve, then your code resumes:

   ```bash
   curl -X POST http://localhost:7421/api/v1/processes/1/resume \
        -H "Authorization: Bearer $token" \
        -H "Content-Type: application/json" \
        -d '{"vars":{"Approver":"Alice"}}'
   ```

   Response:

   ```json
   {
     "instance":1, "status":"completed",
     "vars":{"OrderId":1, "Total":1500, "IsBig":true, "Approver":"Alice"},
     "output":["approver=Alice"]
   }
   ```

   Same `.bpmn`, same step bodies, just suspended in the middle and resumed
   from durable state.

## Concepts to take away

- The same BPMN file runs in two engines: synchronous (modal forms in the
  current process) and headless (suspends + persists for Core).
- `Vars` is a per-instance dictionary — initial values come from the start
  body, every Server step can read and write it, every suspend snapshots it
  to JSON for storage.
- Choice steps are `bpmn:exclusiveGateway` with conditional sequence flows;
  the body returns a string that matches one of the flow conditions.
- HumanTask carries a `<nexor:formRef>` extension that names the form. The
  client (Studio / Flux) opens it; on `Form.Accept()` the engine resumes.

When you're ready to mix server-side execution into the picture, see
[Server-side activities](../../server-side.md).
