---
title: 'Sample projects'
description: 'Three self-contained projects you can open in Studio.'
---

Three self-contained Nexor projects that you can open in Studio
(**File → Open Project**) and run. Each is a full project tree (`.pro`,
`activities/`, `forms/`, `sheets/`, `processes/`) — Studio creates `dist/`
on your first Build.

| Sample | Concepts |
|---|---|
| [`hello-world/`](hello-world/)     | One Activity, one Form, one button. The simplest path through the IDE. |
| [`customer-crud/`](customer-crud/) | Sheet schema + form data binding + entity API + a query. |
| [`order-approval/`](order-approval/) | Sheet + form + a 5-step BPMN process with a Choice gateway and a HumanTask. |

## How to run a sample

1. Launch Studio, **File → Open Project…**, point at the sample's `.pro`.
2. Open the activity from the project tree. Open its main form.
3. **F5** runs the form locally.
4. To exercise the publish loop: **Ctrl+B** Build, then **Ctrl+Shift+P** Publish to Core, then deploy from Command, then run from Flux. (See [Getting Started](../getting-started.md) for the full walkthrough.)

## How to learn from a sample

- Read the `.aba` first — it's the activity definition + Sub source.
- Then the `.frm` — the form's widget tree + event-handler stubs.
- Then any `.sht` — the entity schema.
- Then any `.bpmn` — the process model (open in Studio's Diagram view for the picture, or any text editor for the XML).

Each sample has its own README with a guided tour.
