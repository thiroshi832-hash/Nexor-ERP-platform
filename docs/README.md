# Nexor — Documentation

Nexor is an ERP platform built on a single VB-Script-flavored language and four
desktop apps:

|   | App | Role | Icon |
|---|-----|------|---|
| 🟦 | **Nexor Studio**  | IDE — author Activities, design Forms, define Sheets, model Processes | blue |
| 🟪 | **Nexor Core**    | Backend server — package registry, entity DB, RPC, workflow persistence | purple |
| 🟧 | **Nexor Command** | Admin console — review packages, deploy, roll back, audit | orange |
| 🟩 | **Nexor Flux**    | End-user runtime — install and run published apps | green |

Three apps are GUIs (Studio, Command, Flux) built on Qt 5; Core is a console-mode
HTTP server. They share one source tree under [`/nexor_studio/src/`](../nexor_studio/src/);
Core, Command, and Flux pull in the parts of that tree they need (language,
runtime, package format) via `INCLUDEPATH` so changes to Studio's interpreter
flow through every host on the next rebuild.

## The develop → publish → run loop

```
   ┌──────────┐        ┌──────────┐        ┌──────────┐        ┌──────────┐
   │  Studio  │  build │   Core   │ deploy │  Command │  fetch │   Flux   │
   │ author   │ ─────▶ │ pending  │ ─────▶ │ approves │ ─────▶ │ runs the │
   │ + Build  │        │ live     │        │  +       │        │ activity │
   │          │        │ rolled   │        │ rollback │        │          │
   └──────────┘        └──────────┘        └──────────┘        └──────────┘
```

A `.nexor` package is a single signed XML document containing every Activity,
Form, Sheet, and Process in a project. Studio writes it, Core stores it,
Command flips its status from `pending` to `live`, Flux downloads and runs it.

## Documentation map

Start here, in order:

1. **[Getting Started](getting-started.md)** — prerequisites, build all four apps from source, run end-to-end on one machine.
2. **[Language Reference](language-reference.md)** — every keyword, operator, statement, and built-in function in the Nexor language.
3. **[Forms Guide](forms.md)** — the visual form designer in Studio, widget palette, event handlers, the `Form.*` bridge.
4. **[Sheets & Entities](sheets.md)** — defining schemas (Sheets), the Entity API (`.New / .Find / .All / .Save / .Delete`), LINQ-style queries (`From … Where … Select`).
5. **[Processes & BPMN](processes.md)** — Process activities authored in Studio's BPMN 2.0 designer, step types (Server / Choice / HumanTask / Final), the `Vars` dictionary, persistent workflow instances on Core.
6. **[Publishing & Deployment](publishing.md)** — build a package in Studio, sign with HMAC, publish to Core, review in Command, deploy / roll back.
7. **[Server-side Activities](server-side.md)** — `[Activity(RunsOn := ServerOnly)]`, transparent RPC, the entity REST endpoints.
8. **[Core HTTP API Reference](api-reference.md)** — every endpoint Core exposes, with curl examples.

## Sample projects

Self-contained examples under [`samples/`](samples/) that you can open in Studio:

| Sample | What it teaches |
|---|---|
| [`hello-world/`](samples/hello-world/)         | The smallest possible project: one Activity, one Form, one button. |
| [`customer-crud/`](samples/customer-crud/)     | A Sheet-backed CRUD app with `New / Save / Find / Delete`. |
| [`order-approval/`](samples/order-approval/)   | A BPMN process with a Server step, a Choice gateway, and a HumanTask. |

## Architecture deep dives

Generated PDF reference documents (under [`docs/`](.)):

- [Nexor — Platform Architecture](Nexor-Platform-Architecture.pdf) — the four-app diagram, host adapters, lifecycle, wire protocols.
- [Nexor — Language Architecture](Nexor-Language-Architecture.pdf) — the language design rationale and grammar.
- [Nexor — Multi-Platform Architecture](Nexor-Multi-Platform-Architecture.pdf) — the desktop / browser / Android cross-platform story.

## Status & roadmap

What's shipped today (4 desktop apps, all build clean on Qt 5.14 + mingw730_64):

- [x] Lexer / Parser / tree-walking interpreter
- [x] Form designer + form runner with `Form.Save / .Load / .New / .Delete / .Accept / .Reject`
- [x] Sheet definitions + SQLite-backed Entity ORM + LINQ-style queries
- [x] BPMN 2.0 visual process designer (round-trips with Sparx EA)
- [x] Synchronous + persistent process engines (Server / Choice / HumanTask / Final)
- [x] `.nexor` package format + Studio Build (with HMAC signing)
- [x] Core HTTP server: package registry, audit log, diff endpoint
- [x] Command admin app: master/detail registry UI, deploy/rollback, diff viewer, audit viewer
- [x] Flux desktop runtime: catalog, install, run forms+activities+processes
- [x] `[Activity(RunsOn := ServerOnly)]` + transparent RPC over `/api/v1/rpc/`
- [x] Entity REST CRUD on Core (`/api/v1/entities/:sheet/...`)
- [x] Persistent process instances on Core (`/api/v1/processes/:pkg/:proc/start`, `/resume`, list, get)

Not yet shipped:

- [ ] WebSocket push for HumanTask creation and live package updates
- [ ] ed25519 signing (HMAC works today)
- [ ] Bytecode VM (replace tree-walker)
- [ ] Web SPA + WASM port
- [ ] Android port
- [ ] Reports + scheduling

See the [Platform Architecture PDF](Nexor-Platform-Architecture.pdf), section 14, for the canonical end-to-end build order.
