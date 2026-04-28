# Publishing & Deployment

The lifecycle from a developer's keyboard to a running app on a user's
desktop is split across the four apps:

```
   Studio ──build──▶ .nexor ──signed POST──▶ Core ──pending──▶ Command ──deploy──▶ Core (live) ──fetch──▶ Flux
```

This page walks you through each arrow.

## 1. The `.nexor` package format

A `.nexor` file is a single XML document that bundles every artifact in a
project plus a manifest:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<NexorPackage version="1">
  <Meta>
    <Title>Sales</Title>
    <Id>Sales</Id>
    <Version>0.4.1</Version>
    <BuiltAt>2026-04-29T03:14:07Z</BuiltAt>
    <Builder>Nexor Studio 0.1.0</Builder>
    <Hash algo="sha256">a3f9…64hex…</Hash>
    <Signature algo="hmac-sha256">7c2b…64hex…</Signature>   <!-- optional -->
  </Meta>
  <Activities>
    <Activity id="OrderEntry" file="activities/OrderEntry/OrderEntry.aba">
      <![CDATA[ …the .aba XML, verbatim… ]]>
    </Activity>
  </Activities>
  <Forms>     …Form entries…     </Forms>
  <Sheets>    …Sheet entries…    </Sheets>
  <Processes> …Process entries…  </Processes>
  <Resources> …base64-encoded blobs… </Resources>
</NexorPackage>
```

| Section | What goes there |
|---|---|
| `Meta` | manifest — title, id, version, build timestamp, content hash, optional signature |
| `Activities` | every `.aba` (Activity definition + code) in the project |
| `Forms` | every `.frm` (form XML, full WYSIWYG fidelity) |
| `Sheets` | every `.sht` (entity schema) |
| `Processes` | every `.bpmn` (or legacy `.prc`) |
| `Resources` | base64-encoded binaries (images, …) |

The **Hash** is SHA-256 over a canonical concatenation of every entry's body
in declaration order. Tampering with any artifact invalidates the hash. The
optional **Signature** is HMAC-SHA256 over `id|version|builtAt|hash` keyed by
a shared secret — see [the signing section](#signing) below.

## 2. Build a package in Studio

**Build → Build Package…** (or `Ctrl+B`) prompts for a SemVer version
(remembered per-project across runs) and writes:

```
<projectRoot>/dist/<id>-<version>.nexor
```

Studio also runs the just-written package through `PackageReader` to verify
the hash round-trips. Mismatches surface as a yellow warning in the OUTPUT
pane.

## 3. Sign on build (HMAC-SHA256)

Phase 9b ships HMAC signing as the placeholder for ed25519. Configure via
**Build → Publishing Settings…**:

| Setting | Effect |
|---|---|
| **Signing key** | when set, every Build embeds a `<Signature>` over `id\|version\|builtAt\|hash` keyed by this string |

Core can be started with a matching `--signing-key`; if so it rejects any
upload whose signature doesn't verify (or that lacks one entirely). With no
key on either side, Core stays in permissive mode (development default).

A signed package looks like:

```xml
<Hash algo="sha256">a3f9…</Hash>
<Signature algo="hmac-sha256">7c2b…</Signature>
```

Future Phase 9b' will swap HMAC for ed25519 (libsodium) — same wire shape.

## 4. Publish to Core

**Build → Publish to Core…** (`Ctrl+Shift+P`) reads the most recently built
version from `dist/`, posts the bytes:

```
POST /api/v1/packages
Content-Type: application/x-nexor-package
Authorization: Bearer <admin-token>

<binary .nexor body>
```

Core's response (200):

```json
{
  "id":"Sales", "version":"0.4.1", "title":"Sales",
  "hash":"a3f9...", "byte_size":4738,
  "built_at":"2026-04-29T03:14:07Z",
  "received_at":"2026-04-29T03:14:08Z",
  "status":"pending"
}
```

The fresh row enters as **pending** — never live until an administrator
deploys it.

### What can go wrong

| HTTP | Body | Meaning |
|---|---|---|
| 400 | `{"error":"Bad package: …"}`        | malformed XML or wrong root element |
| 400 | `{"error":"Hash mismatch: …"}`     | signature/hash check failed |
| 400 | `{"error":"Package is unsigned …"}` | Core requires signing, package didn't carry one |
| 401 | `{"error":"Missing or malformed Authorization header."}` | missing bearer |
| 401 | `{"error":"Invalid bearer token."}` | wrong bearer |

## 5. Review and deploy in Command

Launch **NexorCommand**. The master/detail UI shows every package version,
grouped by project, with status colour-coded:

```
PACKAGES                       Sales / 0.4.1
  ▼ Sales                      Status:  pending
     0.4.1                     Title:   Sales
     0.4.0 *  (live)           Built:   2026-04-29T03:14:07Z
  ▼ Inventory                  Hash:    a3f9...
     1.0.0    (live)           Bytes:   4,738

[ Deploy ]  [ Rollback ]  [ Compare with… ]  [ Download… ]  [ Delete pending ]
```

Before clicking **Deploy**, **Compare with…** opens a side-by-side diff
against any other version of the same project:

- **Entries** pane — what got added / removed / changed across Activities,
  Forms, Sheets, Processes, Resources.
- **Schema migration** pane — per-Sheet field changes (added / removed /
  type-changed). This is your "ALTER TABLE risk" preview.

**▲ Deploy** flips the row's status to `live` and demotes any previously-live
row of the same project to `rolled_back`. There is exactly one live version
per project at any time.

**▼ Rollback** flips a `live` row back to `rolled_back`. Note: rollback
doesn't promote the previous live version — the project has no live version
afterwards. To return to the previous version, deploy it explicitly.

### What lands in the audit log

Every state-changing action writes one row to the `audit_events` table:

| Event type        | When |
|-------------------|------|
| `publish`         | a new version is uploaded |
| `deploy`          | status flips to `live` |
| `rollback`        | a live version flips to `rolled_back` |
| `delete-pending`  | a `pending` version is permanently removed |

Each row carries `(occurred_at, actor, package_id, version, detail)`. Actor
is the last six characters of the bearer token (so secrets aren't stored in
the clear). View it in Command via **Audit → Show full audit log…**.

## 6. End-user fetch in Flux

Flux's catalog automatically lists every package on Core. Selecting one and
clicking **⤓ Install** downloads the bytes to:

```
%APPDATA%\NexorFlux\packages\<id>\<ver>.nexor
```

…then extracts to the canonical Project layout under:

```
%APPDATA%\NexorFlux\extracted\<id>\<ver>\
    <id>.pro
    activities/<aid>/<aid>.aba
    activities/<aid>/*.frm
    sheets/<sid>/<sid>.sht
    processes/<pid>/<pid>.bpmn
```

This layout is identical to what Studio writes for a project on disk, so the
existing `Project::load` + `FormRunner` + `ProcessEngine` runs the package
unchanged. Click **▶ Run** to open the Activity Picker.

## 7. Bumping versions

Versioning is SemVer:

- **patch** — bug fix, no surface change → `0.4.1 → 0.4.2`
- **minor** — backward-compatible additions → `0.4.1 → 0.5.0`
- **major** — breaking schema or API change → `0.4.1 → 1.0.0`

Studio remembers the last version per project via `QSettings`. The Build
dialog pre-fills the previous value so the typical motion is: edit `0.4.1`
to `0.4.2`, hit Enter.

## 8. Trust matrix

| Action               | Who can do it | Auth |
|----------------------|--------------|------|
| Build a `.nexor`     | any developer | none — local build |
| Sign a `.nexor`      | anyone with the signing key | shared secret today; ed25519 private key in Phase 9b' |
| Publish to Core      | anyone with a valid bearer token | `Authorization: Bearer <token>` |
| Deploy / rollback    | same bearer | future: a separate `nexor.administrator` role |
| GET catalog          | anyone (open) | no token required for `/api/v1/packages` |
| GET admin catalog    | bearer | `/api/v1/admin/packages` is gated |

In Phase 9c+ the bearer token will be split per role and per tenant — the
HMAC signature stays the developer-team trust anchor.

## 9. CI/CD

Studio's Build is a Qt menu action today, but `PackageBuilder::buildAndWrite`
is callable from any Qt program. A small CLI tool that imports `Project` +
`PackageBuilder` and emits `dist/<id>-<v>.nexor` is on the Phase 8c roadmap;
in the meantime, scripted publish from CI looks like:

```bash
# 1. Build the package by running Studio (or use a future CLI).
# 2. Push it to Core:
curl -X POST https://core.example.com/api/v1/packages \
     -H "Authorization: Bearer $NEXOR_PUBLISH_TOKEN" \
     -H "Content-Type: application/x-nexor-package" \
     --data-binary @dist/Sales-0.4.1.nexor
```

## 10. Where to read next

- **[Server-side Activities](server-side.md)** — `[Activity(RunsOn := ServerOnly)]` and how RPC pulls a published package's code into Core's runtime.
- **[API Reference](api-reference.md)** — every endpoint in `/api/v1/*`.
- **[Getting Started](getting-started.md)** — end-to-end loop, with screenshots of each Studio / Command / Flux step.
