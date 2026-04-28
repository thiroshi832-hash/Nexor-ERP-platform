// =============================================================================
// WidgetFactory — maps Nexor widget-type strings to real Qt widget instances.
//
// Used by both:
//   * FormCanvas — design-time creation when the user drops a palette item.
//   * FormRunner — run-time creation when the user hits Run.
//
// Because both use the SAME factory, the design-time appearance is byte-for-
// byte the same as what the user sees when the form is run.
// =============================================================================
#ifndef NEXOR_STUDIO_WIDGETFACTORY_H
#define NEXOR_STUDIO_WIDGETFACTORY_H

#include <QString>
#include <QVariant>
#include <QSize>

class QWidget;

class WidgetFactory {
public:
    // Creates a default-styled widget of the given type. Caller takes ownership.
    static QWidget *create(const QString &type, QWidget *parent = nullptr);

    // Apply / read a property by name.  Currently supported: "text".
    static void     applyProperty(QWidget *w, const QString &key, const QVariant &value);
    static QVariant readProperty (const QWidget *w, const QString &key);

    // True if this type natively has a settable text property.
    static bool     hasTextProperty(const QString &type);

    // Default geometry for a freshly-dropped widget of this type.
    static QSize    defaultSize(const QString &type);

    // Auto-name prefix per type (Button → btn, Label → lbl, etc.).
    static QString  namePrefix(const QString &type);

    // VB-style "default event" — what double-clicking the control creates.
    static QString  defaultEvent(const QString &type);

    // All event names this control type emits at runtime (used by the
    // PropertyPanel's Events section + the Procedure dropdown).
    static QStringList eventsFor(const QString &type);
};

#endif // NEXOR_STUDIO_WIDGETFACTORY_H
