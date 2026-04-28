// =============================================================================
// Value — Nexor's runtime value type.  Variant-style; the interpreter doesn't
// know typed locals yet, so everything is a Value.
// =============================================================================
#ifndef NEXOR_STUDIO_LANG_VALUE_H
#define NEXOR_STUDIO_LANG_VALUE_H

#include <QString>
#include <QVariant>

namespace nx {

class Value {
public:
    enum Kind { Empty, Bool, Long, Double, String };

    Value();                                  // Empty
    static Value boolean(bool b);
    static Value integer(qint64 n);
    static Value real(double d);
    static Value text(const QString &s);
    static Value nothing();                   // alias for Empty

    Kind     kind()    const { return m_kind; }
    bool     isEmpty() const { return m_kind == Empty; }
    bool     isNumeric() const { return m_kind == Long || m_kind == Double; }

    bool     toBool()   const;
    qint64   toLong()   const;
    double   toDouble() const;
    QString  toText()   const;                // human-printable string (no quotes)

    // Comparisons return -1, 0, +1.  Mixed types coerce numerically when
    // possible, otherwise compare as strings.
    static int compare(const Value &a, const Value &b);

    // Arithmetic helpers used by the interpreter.
    static Value add(const Value &a, const Value &b);   // numeric add OR & concat
    static Value sub(const Value &a, const Value &b);
    static Value mul(const Value &a, const Value &b);
    static Value div(const Value &a, const Value &b);   // floating
    static Value mod(const Value &a, const Value &b);   // integer
    static Value concat(const Value &a, const Value &b);

private:
    Kind     m_kind { Empty };
    qint64   m_i    { 0 };
    double   m_d    { 0 };
    QString  m_s;
};

} // namespace nx

#endif // NEXOR_STUDIO_LANG_VALUE_H
