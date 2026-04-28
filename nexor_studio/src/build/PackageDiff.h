// =============================================================================
// PackageDiff — compares two parsed Packages entry-by-entry and yields a
// machine-friendly description used by Command's diff viewer and by Core's
// schema-migration preview.
//
// Living in nexor_studio/src/build/ so Studio, Core, and Command all link the
// same source — diff semantics are part of the format contract, not a host
// concern.
// =============================================================================
#ifndef NEXOR_STUDIO_PACKAGEDIFF_H
#define NEXOR_STUDIO_PACKAGEDIFF_H

#include <QString>
#include <QVector>
#include "Package.h"

namespace nx {

struct EntryChange {
    enum Kind { Added, Removed, Changed, Unchanged };
    Kind    kind;
    QString section;       // "Activities" | "Forms" | "Sheets" | "Processes" | "Resources"
    QString id;            // entry id (or file for resources)
    QString fromHash;      // sha256 of `fromContent`, empty when Added
    QString toHash;        // sha256 of `toContent`,   empty when Removed
};

// Per-sheet field diff — drives the schema-migration preview.
struct SheetFieldChange {
    enum Kind { Added, Removed, TypeChanged, FlagsChanged };
    Kind    kind;
    QString fieldName;
    QString fromType;       // empty when Added
    QString toType;         // empty when Removed
    QString detail;         // "key true→false", "required false→true", …
};

struct SheetChange {
    QString sheetId;
    QVector<SheetFieldChange> fields;
};

struct PackageDiff {
    QVector<EntryChange>  entries;
    QVector<SheetChange>  sheets;        // schema-migration preview

    int counts(EntryChange::Kind k) const;
};

class PackageDiffer {
public:
    // Compute entry- and field-level diff between two parsed packages.
    static PackageDiff compute(const Package &from, const Package &to);
};

} // namespace nx

#endif // NEXOR_STUDIO_PACKAGEDIFF_H
