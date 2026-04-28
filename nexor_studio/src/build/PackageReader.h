// =============================================================================
// PackageReader — parses a .nexor file back into an in-memory Package.
//
// This is the host-agnostic loader that Flux, Command, and Core will all
// reuse: read the bytes, validate the hash, hand back the Activities /
// Forms / Sheets / Processes / Resources.  The .nexor format is exactly
// what PackageBuilder emits — see Package.h for the schema.
//
// The reader does NOT touch a Project on disk.  Hosts like Flux take the
// resulting Package and feed it directly into in-memory loaders (no
// extraction step).  Studio uses the reader during the build verification
// pass to confirm that what was just written is round-trippable.
// =============================================================================
#ifndef NEXOR_STUDIO_PACKAGEREADER_H
#define NEXOR_STUDIO_PACKAGEREADER_H

#include <QString>
#include <QByteArray>
#include "Package.h"

namespace nx {

class PackageReader {
public:
    enum class Status {
        Ok,
        FileNotFound,
        ParseError,
        VersionMismatch,
        HashMismatch,
        SignatureMissing,
        SignatureMismatch
    };

    struct Result {
        Status   status { Status::Ok };
        QString  message;        // human-readable details
        Package  package;        // populated on Ok and on HashMismatch
    };

    // Read a .nexor file and parse its contents.  When `verifyHash` is true
    // (the default) the function recomputes the SHA-256 over the loaded
    // entries and compares against the manifest hash; mismatch surfaces as
    // Status::HashMismatch but the package is still returned for callers
    // that want to inspect it.
    static Result fromFile(const QString &path, bool verifyHash = true);

    // Same, but parses from an in-memory byte buffer (used by Flux when it
    // streams a package over the network).
    static Result fromBytes(const QByteArray &bytes, bool verifyHash = true);

    // Verifies the manifest's <Signature> against `key` (HMAC-SHA256).  Sets
    // result.status to SignatureMissing if the package was unsigned and
    // `key` is non-empty, or SignatureMismatch on hash drift.  Pass an empty
    // key to skip the check (kept for hosts that don't enforce signing).
    static Status verifySignature(const Package &pkg, const QByteArray &key,
                                  QString *messageOut = nullptr);
};

} // namespace nx

#endif // NEXOR_STUDIO_PACKAGEREADER_H
