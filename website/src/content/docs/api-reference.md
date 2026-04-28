---
title: 'HTTP API reference'
description: 'Every Core endpoint with curl examples.'
---

This page documents every endpoint Nexor Core exposes. Base URL is whatever
you started Core on (`http://localhost:7421` by default).

All endpoints respond with JSON unless explicitly noted. All `POST`/`PATCH`/
`DELETE` and admin reads require `Authorization: Bearer <admin-token>`
(when Core was started with `--admin-token`); GETs of public endpoints do
not.

Errors come back as `{"error": "<message>"}` with an appropriate HTTP code.

## Public endpoints

### `GET /api/v1/health`

Sanity / uptime probe. Always open.

```bash
curl http://localhost:7421/api/v1/health
```

```json
{"service":"NexorCore","version":"0.1.0","uptime":1342}
```

### `GET /api/v1/packages`

List every package row in the registry — pending, live, and rolled-back.
No auth required (clients like Flux are anonymous catalog browsers).

```bash
curl http://localhost:7421/api/v1/packages
```

```json
[
  {"id":"Sales","version":"0.4.1","title":"Sales",
   "hash":"a3f9...","hash_algo":"sha256","byte_size":4738,
   "built_at":"2026-04-29T03:14:07Z","received_at":"2026-04-29T03:14:08Z",
   "status":"pending"},
  {"id":"Sales","version":"0.4.0","status":"live", ...}
]
```

### `GET /api/v1/packages/:id/:version`

Stream the raw `.nexor` bytes for one specific version.

```bash
curl -o sales.nexor http://localhost:7421/api/v1/packages/Sales/0.4.1
```

`200` returns the bytes with `Content-Type: application/x-nexor-package`.
`404` if the (id, version) pair doesn't exist.

## Publish + admin endpoints

### `POST /api/v1/packages`  *(bearer)*

Upload a `.nexor`. Core validates the manifest (hash + signature when
`--signing-key` is set) and writes a new `pending` row.

```bash
curl -X POST http://localhost:7421/api/v1/packages \
     -H "Authorization: Bearer $token" \
     -H "Content-Type: application/x-nexor-package" \
     --data-binary @dist/Sales-0.4.1.nexor
```

`200` → the freshly-stored row (same shape as `GET /api/v1/packages`).
`400` if the package fails to parse, hash check, or signature check.
`401` for missing / wrong bearer.

### `GET /api/v1/admin/packages[?status=…]`  *(bearer)*

Same shape as `/api/v1/packages` but admin-gated. Optional `status` filter:
`pending`, `live`, `rolled_back`.

```bash
curl -H "Authorization: Bearer $token" \
     'http://localhost:7421/api/v1/admin/packages?status=pending'
```

### `GET /api/v1/admin/packages/:id/history`  *(bearer)*

All versions of one project, newest first.

```bash
curl -H "Authorization: Bearer $token" \
     http://localhost:7421/api/v1/admin/packages/Sales/history
```

### `GET /api/v1/admin/packages/:id/diff?from=A&to=B`  *(bearer)*

Compare two versions of the same project. Returns:

```json
{
  "from":"0.4.0",
  "to":"0.4.1",
  "entries":[
    {"kind":"changed", "section":"Sheets",   "id":"Customer",
     "from_hash":"...", "to_hash":"..."},
    {"kind":"added",   "section":"Forms",    "id":"OrderEntry_main",
     "from_hash":"",    "to_hash":"..."}
  ],
  "sheets":[
    {"sheet_id":"Customer", "fields":[
      {"kind":"added",       "name":"Email", "to_type":"String"},
      {"kind":"type-changed","name":"Name",  "from_type":"String", "to_type":"Variant"}
    ]}
  ]
}
```

The `sheets` array drives Command's "schema migration preview".

### `POST /api/v1/admin/packages/:id/:version/deploy`  *(bearer)*

Promote a row to `live`. Demotes any previously-live row of the same id to
`rolled_back`. Writes a `deploy` audit event.

```bash
curl -X POST -H "Authorization: Bearer $token" \
     http://localhost:7421/api/v1/admin/packages/Sales/0.4.1/deploy
```

### `POST /api/v1/admin/packages/:id/:version/rollback`  *(bearer)*

Demote a `live` row to `rolled_back`. Writes a `rollback` audit event.
The project has no `live` version afterwards — to revert, deploy the
previous version explicitly.

### `DELETE /api/v1/admin/packages/:id/:version`  *(bearer)*

Permanently delete a `pending` row from disk + DB. Refuses live or
rolled-back rows. Writes a `delete-pending` audit event.

```bash
curl -X DELETE -H "Authorization: Bearer $token" \
     http://localhost:7421/api/v1/admin/packages/Sales/0.4.1
```

### `GET /api/v1/admin/audit[?package=…]`  *(bearer)*

The append-only audit log, newest first, capped at 500 rows.

```bash
curl -H "Authorization: Bearer $token" \
     'http://localhost:7421/api/v1/admin/audit?package=Sales'
```

```json
[
  {"id":12, "event_type":"deploy", "package_id":"Sales", "version":"0.4.1",
   "actor":"…tok123", "detail":"from=pending to=live",
   "occurred_at":"2026-04-29T03:14:09Z"}
]
```

## RPC endpoint

### `POST /api/v1/rpc/:package/:sub`  *(bearer)*

Invoke a named Sub or Function in the live version of `:package`. Only
`[Activity(RunsOn := ServerOnly)]` and `[Either]` (default) bodies execute;
`[ClientOnly]` returns 500 with the matching error.

Body:

```json
{ "args": [ /* JSON-encoded positional args */ ] }
```

Response:

```json
{
  "result": <any JSON value>,
  "output": [ "Print line 1", "Print line 2", ... ]
}
```

Any error in the called body returns 500 with:

```json
{
  "error":  "<message>",
  "output": [ "...", "..." ]
}
```

`404` if the package has no live version, or no matching Sub.

```bash
curl -X POST http://localhost:7421/api/v1/rpc/Sales/MultiplyByTax \
     -H "Authorization: Bearer $token" \
     -H "Content-Type: application/json" \
     -d '{"args":[100]}'
```

```json
{"result":121,"output":[]}
```

## Entity endpoints

All entity endpoints take a `:sheet` path parameter — the Sheet's `Id` from
its `.sht` (case-sensitive). Core auto-registers schemas on every package
publish so a Sheet is queryable as soon as it lands.

### `GET /api/v1/entities/:sheet`  *(bearer)*

```bash
curl -H "Authorization: Bearer $token" \
     http://localhost:7421/api/v1/entities/Customer
```

```json
[
  {"id":1, "$sheet":"Customer", "fields":{"id":1, "name":"Acme", "balance":1234.56}},
  {"id":2, "$sheet":"Customer", "fields":{"id":2, "name":"Globex", "balance":42}}
]
```

### `GET /api/v1/entities/:sheet/:id`  *(bearer)*

`200` with one row, or `404`.

### `POST /api/v1/entities/:sheet`  *(bearer)*

Create. Body accepts `{fields:{...}}` or `{...}` directly:

```bash
curl -X POST -H "Authorization: Bearer $token" \
     -H "Content-Type: application/json" \
     -d '{"Name":"Acme","Balance":1234.56}' \
     http://localhost:7421/api/v1/entities/Customer
```

```json
{"id":1, "$sheet":"Customer", "fields":{"id":1, "name":"Acme", "balance":1234.56}}
```

`201 Created` on success. Server allocates the `Id` and writes it back into the row.

### `PATCH /api/v1/entities/:sheet/:id`  *(bearer)*

Partial update. Only the keys you send are written.

```bash
curl -X PATCH -H "Authorization: Bearer $token" \
     -H "Content-Type: application/json" \
     -d '{"fields":{"Balance":999.99}}' \
     http://localhost:7421/api/v1/entities/Customer/1
```

### `DELETE /api/v1/entities/:sheet/:id`  *(bearer)*

```bash
curl -X DELETE -H "Authorization: Bearer $token" \
     http://localhost:7421/api/v1/entities/Customer/2
```

```json
{"ok":true, "id":2}
```

## Process endpoints

### `POST /api/v1/processes/:package/:process/start`  *(bearer)*

Run the named Process from its start step. Body carries the initial Vars
bag (optional — empty if omitted).

```bash
curl -X POST http://localhost:7421/api/v1/processes/Sales/OrderApproval/start \
     -H "Authorization: Bearer $token" \
     -H "Content-Type: application/json" \
     -d '{"vars":{"OrderId":42}}'
```

`201 Created`:

```json
{
  "instance":1,
  "package_id":"Sales", "process_id":"OrderApproval",
  "status":"awaiting_human",
  "awaiting_step":"ManagerApprove",
  "awaiting_form":"ApproveOrder.frm",
  "vars":{"OrderId":42, "Total":250},
  "last_error":"",
  "output":["Validated order 42"]
}
```

Possible final states:

- `awaiting_human` — engine paused on a HumanTask
- `completed` — reached a Final step
- `failed` — a runtime error; `last_error` is populated

### `POST /api/v1/processes/:instance/resume`  *(bearer)*

Continue a paused instance. Vars in the body are merged on top of the
persisted bag before the engine resumes.

```bash
curl -X POST http://localhost:7421/api/v1/processes/1/resume \
     -H "Authorization: Bearer $token" \
     -H "Content-Type: application/json" \
     -d '{"vars":{"Approver":"Alice"}}'
```

`400` if the instance isn't in `awaiting_human` state.

### `GET /api/v1/processes[?status=…]`  *(bearer)*

List instances. Optional `status` filter:

```bash
curl -H "Authorization: Bearer $token" \
     'http://localhost:7421/api/v1/processes?status=awaiting_human'
```

### `GET /api/v1/processes/:instance`  *(bearer)*

Single-instance lookup. `404` if not found.

## Reserved endpoints (not yet shipped)

These appear in the architecture (page 8) but aren't implemented today:

- `WS  /api/v1/stream` — WebSocket for `package-published`, `entity-changed`, `workflow-step`, `admin-event` notifications.
- `POST /api/v1/auth/login` — OAuth2 token exchange.

Phase 9c+ will introduce them; the wire shape stays compatible with what's
listed here.

## Versioning

The `/api/v1/` prefix is permanent — incompatible changes will land at
`/api/v2/`. Within v1, additive changes (new fields, new endpoints) ship in
patch releases; removed fields trigger a major bump.

## Where to read next

- **[Server-side activities](server-side.md)** — when to call RPC vs direct entity REST.
- **[Publishing](publishing.md)** — what happens before the rows you query through this API exist.
