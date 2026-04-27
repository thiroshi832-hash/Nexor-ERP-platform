// =============================================================================
// DesignerView — composite Design-mode widget:
//   [WidgetPalette | FormCanvas (in scroll area) | PropertyPanel]
// =============================================================================
#ifndef NEXOR_STUDIO_DESIGNERVIEW_H
#define NEXOR_STUDIO_DESIGNERVIEW_H

#include <QWidget>

class WidgetPalette;
class FormCanvas;
class PropertyPanel;

class DesignerView : public QWidget {
    Q_OBJECT
public:
    explicit DesignerView(QWidget *parent = nullptr);

    FormCanvas    *formCanvas()    const { return m_canvas; }
    WidgetPalette *widgetPalette() const { return m_palette; }
    PropertyPanel *propertyPanel() const { return m_props; }

private:
    WidgetPalette *m_palette;
    FormCanvas    *m_canvas;
    PropertyPanel *m_props;
};

#endif // NEXOR_STUDIO_DESIGNERVIEW_H
