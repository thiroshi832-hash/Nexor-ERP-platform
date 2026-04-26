// =============================================================================
// WidgetPalette — left-side toolbox in Design mode.
//
// Categorised list (Common Controls / Containers / Display / Inputs).  Each
// leaf item has a hand-drawn 16x16 icon and the widget type name.  Emits
// widgetSelected(type) on double-click; future drag-and-drop will instantiate
// the widget on the form canvas.
// =============================================================================
#ifndef NEXOR_STUDIO_WIDGETPALETTE_H
#define NEXOR_STUDIO_WIDGETPALETTE_H

#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;

class WidgetPalette : public QWidget {
    Q_OBJECT
public:
    explicit WidgetPalette(QWidget *parent = nullptr);

signals:
    void widgetSelected(const QString &type);

private:
    QTreeWidgetItem *addCategory(const QString &name);
    void             addWidget(QTreeWidgetItem *category,
                               const QString &type,
                               const QString &description = QString());
    QPixmap          makeIcon(const QString &type) const;

    QTreeWidget *m_tree;
};

#endif // NEXOR_STUDIO_WIDGETPALETTE_H
