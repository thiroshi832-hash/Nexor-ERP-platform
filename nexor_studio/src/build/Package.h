// =============================================================================
// .nexor — the universal Nexor package format.
//
// A single signed XML document that carries everything one host needs to run
// a project: every Activity (.aba), every Form (.frm), every Sheet (.sht),
// every Process (.prc), plus arbitrary opaque resources, plus a manifest
// describing the project, version, and content hash.
//
//     <NexorPackage version="1">
//       <Meta>
//         <Title>Sales</Title>
//         <Id>Sales</Id>
//         <Version>0.4.1</Version>
//         <BuiltAt>2026-04-29T03:14:07Z</BuiltAt>
//         <Builder>Nexor Studio 0.1.0</Builder>
//         <Hash algo="sha256">…64 hex chars…</Hash>
//       </Meta>
//       <Activities>
//         <Activity id="OrderEntry" file="activities/OrderEntry/OrderEntry.aba">
//           <![CDATA[ …full .aba XML… ]]>
//         </Activity>
//         …
//       </Activities>
//       <Forms>     …Form entries…     </Forms>
//       <Sheets>    …Sheet entries…    </Sheets>
//       <Processes> …Process entries… </Processes>
//       <Resources> …base64-encoded blobs… </Resources>
//     </NexorPackage>
//
// The Hash is SHA-256 over a canonical concatenation of every entry's body
// (in declaration order).  Tampering with any artifact invalidates the hash;
// signing (Phase 8b) wraps the hash in a vendor-private ed25519 signature.
//
// PackageBuilder writes the document; PackageReader parses it back into the
// host-agnostic data structures Flux / Core / Command will all consume.
// =============================================================================
#ifndef NEXOR_STUDIO_PACKAGE_H
#define NEXOR_STUDIO_PACKAGE_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QDateTime>
#include <QByteArray>

namespace nx {

struct PackageEntry {
    QString id;            // logical id (e.g. "OrderEntry" for an Activity)
    QString file;          // original project-relative path
    QString content;       // textual artifact body (verbatim XML/source)
};

struct ResourceEntry {
    QString file;          // original project-relative path
    QByteArray data;       // raw bytes (base64-encoded on the wire)
};

struct PackageMeta {
    QString   title;
    QString   id;          // project id
    QString   version;     // SemVer string, e.g. "0.4.1"
    QDateTime builtAt;
    QString   builder;     // tool that produced the package
    QString   hash;        // sha256 hex over canonical content
    QString   hashAlgo { "sha256" };

    // Signature over the canonical content + manifest hash.  Empty for
    // unsigned packages (acceptable to a Core started without --signing-key).
    QString   signature;             // hex digest, currently HMAC-SHA256
    QString   sigAlgo  { "hmac-sha256" };
};

// Whole-package payload — what PackageBuilder writes and PackageReader yields.
struct Package {
    PackageMeta              meta;
    QVector<PackageEntry>    activities;
    QVector<PackageEntry>    forms;
    QVector<PackageEntry>    sheets;
    QVector<PackageEntry>    processes;
    QVector<ResourceEntry>   resources;
};

} // namespace nx

#endif // NEXOR_STUDIO_PACKAGE_H
