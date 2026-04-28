# Server-side Activities

The architecture's split-execution promise (Platform-Architecture.pdf,
page 6) — _"one bytecode, multiple hosts"_ with a `RunsOn` annotation that
the compiler enforces — is real, today. Subs marked `[Activity(RunsOn :=
ServerOnly)]` execute on Core; everything else stays on the client.

This page covers:

1. The annotation syntax.
2. How RPC routing works.
3. Calling entity REST endpoints directly.
4. Kicking off a process from client code.

## 1. Annotation

```vbnet
[Activity(RunsOn := ServerOnly, RequiresPermission := "Sales.Approve")]
Public Function ApproveOrder(orderId As Long) As Boolean
    Dim ord = Order.Find(orderId)
    If ord Is Nothing Then Return False
    ord.Approved   = True
    ord.ApprovedAt = Now()
    Return ord.Save()
End Function

[Activity(RunsOn := ClientOnly)]
Public Sub txtSearch_Change()
    Form.lblHint.Text = "Type at least 3 chars"
End Sub

[Activity(RunsOn := Either)]                ' default - omittable
Public Function FormatTotal(amt As Decimal) As String
    Return "$" & amt
End Function
```

| Value         | Meaning |
|---------------|---------|
| `ServerOnly`  | Always runs on Core. Client interpreters route through the RPC bridge. |
| `ClientOnly`  | Always runs on the host (Studio's `▶ Run Form`, Flux). Core refuses the call with `[ClientOnly] '<name>' cannot run on the server.` |
| `Either`      | Runs wherever invoked. Default for un-annotated subs. |

The annotation map is preserved through the package format and re-parsed on
every host that compiles the activity. There's no separate "stub" code;
client and server load the same source — they just behave differently for
the same `Sub` based on host role.

## 2. How RPC routing works

The interpreter has three host roles:

- `Either` (default — Studio + tests; everything runs locally regardless)
- `Client` (Flux — `[ServerOnly]` triggers the RPC bridge)
- `Server` (Core — `[ClientOnly]` errors)

When a `[ServerOnly]` Sub is invoked from a `Client` interpreter:

```
   Sub call: ApproveOrder(7)
        │
        ▼
   Interpreter::call()
        │  reads SubDecl::isServerOnly() == true
        │  reads m_hostRole == Client
        │  reads m_rpcBridge != null
        ▼
   m_rpcBridge("ApproveOrder", [7])
        │
        ▼
   POST /api/v1/rpc/<package>/ApproveOrder
        Authorization: Bearer <token>
        body: {"args":[7]}
        │
        ▼
   Core RpcApi → loads live <package> → spins up server interpreter →
                 calls ApproveOrder(7) → returns its Value
        │
        ▼
   { "result": true, "output": [...] }
        │
        ▼
   Interpreter receives `result`, decodes, returns to caller
```

The whole flow is invisible to the calling Sub — `Dim ok = ApproveOrder(7)`
looks identical for client and server execution. Only the `[ServerOnly]`
annotation changes which side the body runs on.

### Argument and return value JSON envelope

| Nexor `Value` | JSON |
|---|---|
| `Empty` | `null` |
| `Boolean` | `true` / `false` |
| `Long` | integer number |
| `Double` | decimal number |
| `String` | string |
| `List` | array (recursively) |
| `Entity` | `{"$kind":"Entity", "$id":N, "$sheet":"…", "fields":{…}}` |

`Sheet`, `Process`, `Form`, and `Vars` objects do not round-trip — pass
primitives, lists, or entities. The full mapping lives in
[`ValueJson.cpp`](../nexor_core/src/ValueJson.cpp).

## 3. Entity REST CRUD

Once a package is deployed, every Sheet in it is exposed at:

```
GET    /api/v1/entities/:sheet                -> [{id, $sheet, fields:{...}}]
GET    /api/v1/entities/:sheet/:id            -> {id, $sheet, fields:{...}}
POST   /api/v1/entities/:sheet                -> 201 + {id, ...}
PATCH  /api/v1/entities/:sheet/:id            -> {id, ...} (partial)
DELETE /api/v1/entities/:sheet/:id            -> {ok: true, id}
```

All endpoints require `Authorization: Bearer <token>`. Body shape accepts
either `{fields: {...}}` or `{...}` directly:

```bash
# Create a new customer
curl -X POST http://localhost:7421/api/v1/entities/Customer \
     -H "Authorization: Bearer $token" \
     -H "Content-Type: application/json" \
     -d '{"fields":{"Name":"Acme","Balance":1234.56}}'

# Update one field
curl -X PATCH http://localhost:7421/api/v1/entities/Customer/1 \
     -H "Authorization: Bearer $token" \
     -H "Content-Type: application/json" \
     -d '{"fields":{"Balance":999.99}}'
```

In Sub code these aren't usually called directly — `Customer.Find(1)` and
`Customer.Save()` end up at the same SQL via Core's interpreter when the
sub is `[ServerOnly]`. The REST endpoints are for non-Nexor clients
(integration scripts, the future Web SPA).

## 4. Kicking off a process from client code

`<ProcessName>.Start()` runs a registered process. On a host with the
synchronous engine (Studio, Flux), it runs in-process; on a server-coupled
client, it goes over the wire (in the future — today the bridge is
synchronous-only and `<ProcessName>.Start()` always uses the local engine).

To start a server-side persistent process explicitly:

```vbnet
[Activity(RunsOn := ServerOnly)]
Sub StartApproval(orderId As Long)
    ' This Sub runs on Core; calling OrderApproval.Start() here uses the
    ' synchronous engine in-process, but a persistent run is what we want:
    ' use the REST endpoint instead.
End Sub
```

For a true persistent run, post directly:

```bash
# Start an instance.  Returns instance id + initial state.
curl -X POST http://localhost:7421/api/v1/processes/Sales/OrderApproval/start \
     -H "Authorization: Bearer $token" \
     -H "Content-Type: application/json" \
     -d '{"vars":{"OrderId":42}}'
```

The response carries `instance`, `status` (one of `awaiting_human`,
`completed`, `failed`), `awaiting_form`, `awaiting_step`, `vars`. Save the
instance id, show the form named in `awaiting_form` to your user, and on
their submit:

```bash
curl -X POST http://localhost:7421/api/v1/processes/<instance>/resume \
     -H "Authorization: Bearer $token" \
     -H "Content-Type: application/json" \
     -d '{"vars":{"Approver":"Alice"}}'
```

The engine merges your override on top of the persisted vars and runs to
the next pause point or to completion.

## 5. End-to-end example: a server-side approval

`Sales/0.4.1/activities/OrderEntry/OrderEntry.aba`:

```vbnet
' --- Client-side: form glue ---
[Activity(RunsOn := ClientOnly)]
Sub btnSubmit_Click()
    Dim id = CLng(Form.txtOrderId)
    Dim ok = ApproveOrder(id)        ' transparent RPC if [ServerOnly]
    If ok Then
        MsgBox "Approved order " & id
        Form.Close()
    Else
        MsgBox "Approval failed"
    End If
End Sub

' --- Server-side: business rules + DB write ---
[Activity(RunsOn := ServerOnly)]
Function ApproveOrder(orderId As Long) As Boolean
    Dim ord = Order.Find(orderId)
    If ord Is Nothing Then Return False
    If ord.Total > Vars.AutoApproveLimit Then
        ' kick off the persistent OrderApproval process
        OrderApproval.Start()
        Return True
    End If
    ord.Approved   = True
    ord.ApprovedAt = Now()
    Return ord.Save()
End Function

' --- Pure utility: runs wherever ---
[Activity(RunsOn := Either)]
Function FormatMoney(amt) As String
    Return "$" & Round(amt, 2)
End Function
```

When Flux executes `btnSubmit_Click`:

1. `txt OrderId` value is read locally.
2. `ApproveOrder(id)` — the interpreter sees `[ServerOnly]` + role=Client + bridge installed → POST to `/api/v1/rpc/Sales/ApproveOrder`.
3. Core loads the live `Sales` package, compiles its activities into a server-side interpreter (HostRole=Server), runs `ApproveOrder` with `args=[id]` against `core_entities.db`.
4. Returns `{"result": true, "output": [...]}`.
5. Flux's interpreter decodes `true` and resumes.

The user just sees a button click and a message box.

## 6. Permissions

`RequiresPermission` is parsed today but not yet enforced — Phase 9c+ will
wire it through the bearer token's claims. For now, treat it as
documentation.

## 7. Limitations (today)

- The RPC bridge is synchronous (uses `QEventLoop` in Flux). Long-running
  server calls block the UI thread. Phase 14b' will move to async with a
  spinner.
- Print output from a server call comes back in the response's `output`
  array — it doesn't stream. Long server bodies appear as a single batch
  when the call completes.
- No reconnect / retry on RPC errors yet. A failed call surfaces as
  `Empty` to the caller plus a red OUTPUT line.

## 8. Where to read next

- **[API Reference](api-reference.md)** — full schema for every endpoint.
- **[Sheets & Entities](sheets.md)** — what `Customer.Find(7)` resolves to server-side.
- **[Processes & BPMN](processes.md)** — persistent workflows, the `process_instances` table.
