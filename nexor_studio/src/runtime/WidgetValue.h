// =============================================================================
// WidgetValue — reads/writes a Qt widget's "primary value" as a Nexor Value.
//
// Used by FormRunner to push entity field values into bound widgets and pull
// them back out on save.  Per-widget mapping:
//
//   QLineEdit         text         (String)
//   QPlainTextEdit    plainText    (String)
//   QLabel            text         (String)
//   QAbstractButton   text         (String)            — for command buttons
//   QCheckBox         checked      (Boolean)           — overrides text
//   QComboBox         currentText  (String)
//   QSpinBox          value        (Long)
//   QDoubleSpinBox    value        (Double)
//   QSlider           value        (Long)
//   QDateTimeEdit     dateTime     (String, ISO 8601)
//   QProgressBar      value        (Long)
// =============================================================================
#ifndef NEXOR_STUDIO_WIDGETVALUE_H
#define NEXOR_STUDIO_WIDGETVALUE_H

#include "language/Value.h"

class QWidget;

namespace nx {

class WidgetValue {
public:
    static Value read (const QWidget *w);
    static void  write(QWidget *w, const Value &v);
};

} // namespace nx

#endif // NEXOR_STUDIO_WIDGETVALUE_H
