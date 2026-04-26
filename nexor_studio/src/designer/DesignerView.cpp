#include "DesignerView.h"
#include "WidgetPalette.h"
#include "FormDesigner.h"

#include <QSplitter>
#include <QHBoxLayout>

DesignerView::DesignerView(QWidget *parent) : QWidget(parent) {
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->setChildrenCollapsible(false);
    splitter->setStyleSheet("QSplitter::handle{ background:#1e2030; }");

    m_palette  = new WidgetPalette(splitter);
    m_designer = new FormDesigner(splitter);

    splitter->addWidget(m_palette);
    splitter->addWidget(m_designer);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({ 200, 1000 });

    row->addWidget(splitter);

    // Bubble palette double-click selection up so MainWindow can act on it.
    connect(m_palette, &WidgetPalette::widgetSelected,
            this, &DesignerView::widgetRequestedOnCanvas);
}
