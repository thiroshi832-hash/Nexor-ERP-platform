// =============================================================================
// DesignerView — composite widget hosting [WidgetPalette | FormDesigner].
//
// Replaces the bare FormDesigner that used to sit at PageDesigner in the
// CentralStack.  Exposes the inner FormDesigner via formDesigner() so
// MainWindow can still call loadForm()/clearForm() on it.
// =============================================================================
#ifndef NEXOR_STUDIO_DESIGNERVIEW_H
#define NEXOR_STUDIO_DESIGNERVIEW_H

#include <QWidget>

class WidgetPalette;
class FormDesigner;

class DesignerView : public QWidget {
    Q_OBJECT
public:
    explicit DesignerView(QWidget *parent = nullptr);

    FormDesigner  *formDesigner()  const { return m_designer; }
    WidgetPalette *widgetPalette() const { return m_palette; }

signals:
    void widgetRequestedOnCanvas(const QString &type);

private:
    WidgetPalette *m_palette;
    FormDesigner  *m_designer;
};

#endif // NEXOR_STUDIO_DESIGNERVIEW_H
