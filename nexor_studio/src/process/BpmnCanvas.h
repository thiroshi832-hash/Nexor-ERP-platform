// =============================================================================
// BpmnCanvas — visual BPMN 2.0 editor for a Process activity.
//
// Built on QGraphicsView/QGraphicsScene.  Each step renders as a BPMN-shape
// item:
//
//     scriptTask  (Server)     rounded rectangle
//     userTask    (HumanTask)  rounded rectangle, hand glyph
//     exclusiveGateway (Choice) diamond
//     startEvent               thin circle
//     endEvent    (Final)      thick circle
//
// Sequence flows are arrows between item ports.  Drag from a node port to
// another node creates a connection.  Selection drives the inspector panel
// in ProcessEditor (type combo + form picker + Nexor code editor).
// =============================================================================
#ifndef NEXOR_STUDIO_BPMNCANVAS_H
#define NEXOR_STUDIO_BPMNCANVAS_H

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QHash>
#include "project/Process.h"

class QGraphicsScene;

namespace nx {

class BpmnNodeItem;
class BpmnEdgeItem;

class BpmnCanvas : public QGraphicsView {
    Q_OBJECT
public:
    explicit BpmnCanvas(QWidget *parent = nullptr);

    // Replaces the whole scene from the given process.  Layout comes from
    // each StepSpec's pos/size (BPMN DI), with auto-layout fallback for
    // steps that have no recorded position.
    void setProcess(Process *process);
    Process *process() const { return m_process; }

    // Re-renders the diagram from m_process — call after the inspector
    // mutates the model (rename / type change / branch add).
    void rebuild();

    // Push QGraphicsItem positions back into the underlying StepSpecs so
    // the next save() / writeBpmn() emits a faithful layout.
    void syncLayoutToModel();

signals:
    void stepSelected(const QString &stepId);   // empty when nothing is selected
    void modelChanged();                        // any structural edit

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;

private slots:
    void onSelectionChanged();

private:
    void clearScene();
    void layoutAuto();          // first-time layout if positions are missing
    void redrawEdges();
    QPointF nextFreeSlot() const;

    BpmnNodeItem *nodeFor(const QString &stepId) const;

    Process                          *m_process { nullptr };
    QGraphicsScene                   *m_scene;
    QHash<QString, BpmnNodeItem*>     m_nodes;
    QVector<BpmnEdgeItem*>            m_edges;
};

} // namespace nx

#endif // NEXOR_STUDIO_BPMNCANVAS_H
