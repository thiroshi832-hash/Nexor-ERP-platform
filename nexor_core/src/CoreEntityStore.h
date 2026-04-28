// =============================================================================
// CoreEntityStore — server-side wrapper around the language EntityStore
// for the data exposed via /api/v1/entities/*.
//
// On every publish/deploy Core scans the package's <Sheets> entries and
// calls registerSheetsFromPackage() so the underlying SQL store has the
// right tables before any HTTP request lands.  The store opens its own
// SQLite file (core_entities.db) under the registry's data root - kept
// separate from core.db so the package-management metadata never mingles
// with tenant data.
//
// Phase 15 has no auth on these endpoints; Phase 9c-style bearer auth
// will return for Phase 15b.  Tenant routing (architecture page 7) is
// a separate concern that wraps this store.
// =============================================================================
#ifndef NEXOR_CORE_COREENTITYSTORE_H
#define NEXOR_CORE_COREENTITYSTORE_H

#include <QString>
#include "../../nexor_studio/src/build/Package.h"
#include "../../nexor_studio/src/language/EntityStore.h"

namespace nx {

class CoreEntityStore {
public:
    explicit CoreEntityStore(const QString &dataRoot);

    bool open(QString *error = nullptr);
    bool isOpen() const                  { return m_inner.isOpen(); }

    // Register every Sheet in `pkg` with the underlying store.  Idempotent;
    // re-registering the same sheet refreshes its schema (creates new
    // columns; SQLite-limited - DROP COLUMN isn't auto-applied).
    void registerSheetsFromPackage(const Package &pkg);

    EntityStore *inner() { return &m_inner; }

private:
    QString     m_root;
    EntityStore m_inner;
};

} // namespace nx

#endif // NEXOR_CORE_COREENTITYSTORE_H
