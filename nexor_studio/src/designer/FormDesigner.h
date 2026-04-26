// =============================================================================
// FormDesigner — visual canvas that draws a preview of a .frm file.
//
// The canvas paints a centred "form" rectangle (sized to the form's declared
// geometry) with a Windows-style title bar.  The body shows a help-text
// placeholder until the widget palette ships.  Below the form, a footer line
// shows the file path and dimensions.
// =============================================================================
#ifndef NEXOR_STUDIO_FORMDESIGNER_H
#define NEXOR_STUDIO_FORMDESIGNER_H

#include <QWidget>

class FormDesigner : public QWidget {
    Q_OBJECT
public:
    explicit FormDesigner(QWidget *parent = nullptr);

    bool    loadForm(const QString &filePath);
    void    clearForm();
    QString currentFormPath() const { return m_path; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_path;
    QString m_id;
    QString m_title;
    int     m_x { 100 };
    int     m_y { 100 };
    int     m_w { 640 };
    int     m_h { 480 };
    bool    m_loaded { false };
};

#endif // NEXOR_STUDIO_FORMDESIGNER_H
