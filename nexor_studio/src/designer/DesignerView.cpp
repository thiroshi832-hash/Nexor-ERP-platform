#include "DesignerView.h"
#include "WidgetPalette.h"
#include "FormCanvas.h"
#include "PropertyPanel.h"

#include <QSplitter>
#include <QHBoxLayout>
#include <QScrollArea>

DesignerView::DesignerView(QWidget *parent) : QWidget(parent) {
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->setChildrenCollapsible(false);
    splitter->setStyleSheet("QSplitter::handle{ background:#1e2030; }");

    m_palette = new WidgetPalette(splitter);

    // Wrap the canvas in a scroll area so large forms can be panned.
    auto *scroll = new QScrollArea(splitter);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background:#2d2d30; border:none; }");
    scroll->setWidgetResizable(true);
    m_canvas = new FormCanvas(scroll);
    scroll->setWidget(m_canvas);

    m_props = new PropertyPanel(splitter);
    m_props->setCanvas(m_canvas);

    splitter->addWidget(m_palette);
    splitter->addWidget(scroll);
    splitter->addWidget(m_props);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({ 200, 800, 240 });

    row->addWidget(splitter);
}
