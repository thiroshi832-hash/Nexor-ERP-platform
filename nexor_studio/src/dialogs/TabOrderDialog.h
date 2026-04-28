// =============================================================================
// TabOrderDialog — VB6-style Tab Order editor.
//
// Lists every widget on the form in current tab order; user reorders via
// Move Up / Move Down buttons.  OK applies the new order to the FormCanvas.
// =============================================================================
#ifndef NEXOR_STUDIO_TABORDERDIALOG_H
#define NEXOR_STUDIO_TABORDERDIALOG_H

#include <QDialog>
#include <QStringList>

class FormCanvas;
class QListWidget;

class TabOrderDialog : public QDialog {
    Q_OBJECT
public:
    explicit TabOrderDialog(FormCanvas *canvas, QWidget *parent = nullptr);

private slots:
    void onUp();
    void onDown();
    void onAccept();

private:
    FormCanvas  *m_canvas;
    QListWidget *m_list;
};

#endif // NEXOR_STUDIO_TABORDERDIALOG_H
