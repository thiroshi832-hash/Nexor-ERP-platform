// =============================================================================
// PropertyPanel — right-side panel in Design mode.
//
// When a widget on the canvas is selected, displays editable fields for:
//   Type (read-only)
//   Name
//   X, Y, Width, Height
//   Text (when applicable)
//
// Edits are pushed back into the FormCanvas via setNameForSelected etc.
// =============================================================================
#ifndef NEXOR_STUDIO_PROPERTYPANEL_H
#define NEXOR_STUDIO_PROPERTYPANEL_H

#include <QWidget>

class FormCanvas;
class QLabel;
class QLineEdit;
class QSpinBox;
class QFormLayout;

class PropertyPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertyPanel(QWidget *parent = nullptr);

    void setCanvas(FormCanvas *canvas);

public slots:
    void onSelectionChanged(QWidget *w);
    void refreshFromSelection();

private slots:
    void onNameEdited();
    void onTextEdited();
    void onGeometryEdited();

private:
    void setEnabledFields(bool on);

    FormCanvas *m_canvas { nullptr };
    bool        m_updating { false };

    QLabel    *m_typeLabel;
    QLineEdit *m_nameEdit;
    QSpinBox  *m_xSpin;
    QSpinBox  *m_ySpin;
    QSpinBox  *m_wSpin;
    QSpinBox  *m_hSpin;
    QLineEdit *m_textEdit;
    QLabel    *m_emptyLabel;
};

#endif // NEXOR_STUDIO_PROPERTYPANEL_H
