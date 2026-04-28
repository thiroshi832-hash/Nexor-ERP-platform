// =============================================================================
// PackageBuilder — turns a Studio Project into a .nexor file on disk.
//
// Walks every Activity, Form, Sheet, Process, and resource in the project,
// reads their text content, computes a canonical SHA-256 over the lot, and
// writes a single XML document.  The output path is conventionally:
//
//     <projectRoot>/dist/<projectId>-<version>.nexor
//
// Phase 8b will gain a sign() variant that wraps the hash in an ed25519
// signature; for now `Hash` alone is enough to detect tampering and to
// give Command something stable to display.
// =============================================================================
#ifndef NEXOR_STUDIO_PACKAGEBUILDER_H
#define NEXOR_STUDIO_PACKAGEBUILDER_H

#include <QString>
#include "Package.h"

class Project;

namespace nx {

class PackageBuilder {
public:
    struct Result {
        bool       ok      { false };
        QString    error;
        QString    outputPath;
        PackageMeta meta;
    };

    // Builds a Package from the project (reading every artifact off disk),
    // writes it to <projectRoot>/dist/<id>-<version>.nexor, and returns
    // metadata about the result.  `version` is a SemVer string supplied by
    // the user (e.g. "0.4.1").
    //
    // When `signingKey` is non-empty the manifest gains a <Signature> element
    // that any Core started with the matching --signing-key will accept; an
    // unsigned package is also valid against a permissive Core.
    static Result buildAndWrite(const Project &project,
                                const QString &version,
                                const QByteArray &signingKey = {});

    // Lower-level: collects every artifact into an in-memory Package.
    // Useful for tests and for Flux's "run a project directly" path.
    static bool collect(const Project &project, Package &out, QString *error = nullptr);

    // Serialises an in-memory Package to a single XML byte stream.
    // Computes (or re-computes) the manifest hash before writing, and — if
    // `signingKey` is non-empty — the manifest signature too.
    static QByteArray serialise(Package &package, const QByteArray &signingKey = {});
};

} // namespace nx

#endif // NEXOR_STUDIO_PACKAGEBUILDER_H
