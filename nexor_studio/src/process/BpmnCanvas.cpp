#include "BpmnCanvas.h"

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsPathItem>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QMenu>
#include <QInputDialog>
#include <QContextMenuEvent>
#include <QtMath>

namespace nx {

namespace {

constexpr qreal kStepW = 120;
constexpr qreal kStepH = 60;
constexpr qreal kGwSize = 56;
constexpr qreal kEvSize = 36;

QColor fillFor(const QString &type) {
    if (type.compare("Choice",    Qt::CaseInsensitive) == 0) return QColor("#3a2c1a");
    if (type.compare("HumanTask", Qt::CaseInsensitive) == 0) return QColor("#1f2c47");
    if (type.compare("Final",     Qt::CaseInsensitive) == 0) return QColor("#5f1e1e");
    if (type.compare("Start",     Qt::CaseInsensitive) == 0) return QColor("#15391f");
    return QColor("#1e3a5f");      // Server / scriptTask
}
QColor strokeFor(const QString &type) {
    if (type.compare("Choice",    Qt::CaseInsensitive) == 0) return QColor("#fb923c");
    if (type.compare("HumanTask", Qt::CaseInsensitive) == 0) return QColor("#5b8cff");
    if (type.compare("Final",     Qt::CaseInsensitive) == 0) return QColor("#ef4444");
    if (type.compare("Start",     Qt::CaseInsensitive) == 0) return QColor("#22c55e");
    return QColor("#a3e635");
}

} // namespace

// ─── BpmnNodeItem ────────────────────────────────────────────────────────
class BpmnNodeItem : public QGraphicsItem {
public:
    explicit BpmnNodeItem(StepSpec *step) : m_step(step) {
        setFlag(ItemIsMovable, true);
        setFlag(ItemIsSelectable, true);
        setFlag(ItemSendsGeometryChanges, true);
        setAcceptHoverEvents(true);
        setPos(step->pos);
        if (!step->size.isValid()) step->size = defaultSize();
    }

    StepSpec *step() const { return m_step; }
    QString   stepId() const { return m_step->id; }

    QSizeF defaultSize() const {
        QString t = m_step->type.toLower();
        if (t == "choice")          return {kGwSize, kGwSize};
        if (t == "final" || t == "start") return {kEvSize, kEvSize};
        return {kStepW, kStepH};
    }

    QRectF boundingRect() const override {
        QSizeF z = m_step->size.isValid() ? m_step->size : defaultSize();
        return QRectF(-z.width()/2, -z.height()/2, z.width(), z.height())
            .adjusted(-2, -2, 2, 2);
    }

    QPainterPath shape() const override {
        QPainterPath p;
        QSizeF z = m_step->size.isValid() ? m_step->size : defaultSize();
        QString t = m_step->type.toLower();
        QRectF r(-z.width()/2, -z.height()/2, z.width(), z.height());
        if (t == "choice") {
            QPolygonF dia; dia << QPointF(0, -z.height()/2)
                               << QPointF(z.width()/2, 0)
                               << QPointF(0, z.height()/2)
                               << QPointF(-z.width()/2, 0);
            p.addPolygon(dia);
            p.closeSubpath();
        } else if (t == "final" || t == "start") {
            p.addEllipse(r);
        } else {
            p.addRoundedRect(r, 8, 8);
        }
        return p;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *,
               QWidget *) override {
        QSizeF z = m_step->size.isValid() ? m_step->size : defaultSize();
        QString t = m_step->type.toLower();
        QRectF r(-z.width()/2, -z.height()/2, z.width(), z.height());

        QColor f = fillFor(m_step->type);
        QColor s = strokeFor(m_step->type);
        if (isSelected()) s = QColor("#ffffff");

        QPen pen(s, t == "final" ? 4 : 2);
        painter->setPen(pen);
        painter->setBrush(f);

        if (t == "choice") {
            QPolygonF dia; dia << QPointF(0, -z.height()/2)
                               << QPointF(z.width()/2, 0)
                               << QPointF(0, z.height()/2)
                               << QPointF(-z.width()/2, 0);
            painter->drawPolygon(dia);
            painter->setPen(QColor("#fb923c"));
            QFont gf = painter->font();
            gf.setBold(true); gf.setPointSize(14);
            painter->setFont(gf);
            painter->drawText(r, Qt::AlignCenter, "X");
        } else if (t == "final" || t == "start") {
            painter->drawEllipse(r);
        } else {
            painter->drawRoundedRect(r, 8, 8);
            if (t == "humantask") {
                painter->setPen(QColor("#5b8cff"));
                QFont gf = painter->font();
                gf.setPointSize(11);
                painter->setFont(gf);
                painter->drawText(r.adjusted(6, 4, 0, 0).topLeft() + QPointF(0, 12),
                                  "👤");
            }
        }
        painter->setPen(QColor("#dce1e7"));
        QFont lf = painter->font();
        lf.setPointSize(9);
        painter->setFont(lf);
        QString label = m_step->name.isEmpty() ? m_step->id : m_step->name;
        painter->drawText(r, Qt::AlignCenter | Qt::TextWordWrap, label);
    }

    QPointF portFor(const QPointF &otherPos) const {
        QRectF r = boundingRect().adjusted(2, 2, -2, -2);
        QPointF c = scenePos();
        QPointF o = otherPos - c;
        // Clip the ray from centre to bounding-rect edge — simple AABB.
        qreal dx = o.x(), dy = o.y();
        if (dx == 0 && dy == 0) return c;
        qreal ax = qFabs(dx), ay = qFabs(dy);
        qreal hw = r.width()/2, hh = r.height()/2;
        qreal sx = (ax/hw > ay/hh) ? hw/ax : hh/ay;
        return c + QPointF(dx, dy) * sx;
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &v) override {
        if (change == ItemPositionChange) {
            if (m_step) m_step->pos = v.toPointF();
        }
        return QGraphicsItem::itemChange(change, v);
    }

private:
    StepSpec *m_step;
};

// ─── BpmnEdgeItem ────────────────────────────────────────────────────────
class BpmnEdgeItem : public QGraphicsPathItem {
public:
    BpmnEdgeItem(BpmnNodeItem *from, BpmnNodeItem *to,
                 const QString &label, bool dashed = false)
        : m_from(from), m_to(to), m_label(label), m_dashed(dashed) {
        setZValue(-1);
        QPen p(QColor("#8a95a3"), 1.6);
        if (dashed) p.setStyle(Qt::DashLine);
        setPen(p);
        setBrush(Qt::NoBrush);
    }

    BpmnNodeItem *from() const { return m_from; }
    BpmnNodeItem *to()   const { return m_to;   }

    void update() {
        if (!m_from || !m_to) return;
        QPointF a = m_from->portFor(m_to->scenePos());
        QPointF b = m_to  ->portFor(m_from->scenePos());
        QPainterPath path(a);
        path.lineTo(b);
        setPath(path);

        // Arrowhead.
        QPointF d = b - a;
        qreal len = std::hypot(d.x(), d.y());
        if (len < 1e-6) return;
        d /= len;
        QPointF n(-d.y(), d.x());
        QPointF tip   = b;
        QPointF base1 = b - d*10 + n*4;
        QPointF base2 = b - d*10 - n*4;
        QPainterPath arrow(tip);
        arrow.lineTo(base1);
        arrow.lineTo(base2);
        arrow.closeSubpath();
        path.addPath(arrow);
        setPath(path);
    }

    QString label() const { return m_label; }

private:
    BpmnNodeItem *m_from, *m_to;
    QString       m_label;
    bool          m_dashed;
};

// ─── BpmnCanvas ──────────────────────────────────────────────────────────

BpmnCanvas::BpmnCanvas(QWidget *parent) : QGraphicsView(parent) {
    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QColor("#0d0e12"));
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setDragMode(QGraphicsView::RubberBandDrag);
    setStyleSheet(R"(
        QGraphicsView { background:#0d0e12; border:none; }
    )");
    connect(m_scene, &QGraphicsScene::selectionChanged,
            this, &BpmnCanvas::onSelectionChanged);
    connect(m_scene, &QGraphicsScene::changed, this, [this]{ redrawEdges(); });
}

void BpmnCanvas::setProcess(Process *p) {
    m_process = p;
    rebuild();
}

void BpmnCanvas::clearScene() {
    m_nodes.clear();
    m_edges.clear();
    m_scene->clear();
}

QPointF BpmnCanvas::nextFreeSlot() const {
    int n = m_nodes.size();
    return QPointF(140 + (n * 160) % 800, 160 + 100 * ((n * 160) / 800));
}

void BpmnCanvas::layoutAuto() {
    int i = 0;
    for (auto &s : m_process->steps()) {
        if (s.pos.isNull()) {
            s.pos = QPointF(120 + 160 * i, 160);
        }
        ++i;
    }
}

void BpmnCanvas::rebuild() {
    clearScene();
    if (!m_process) return;
    layoutAuto();

    // Nodes
    for (StepSpec &s : m_process->steps()) {
        auto *item = new BpmnNodeItem(&s);
        m_scene->addItem(item);
        m_nodes.insert(s.id, item);
    }
    // Edges
    for (const StepSpec &s : m_process->steps()) {
        BpmnNodeItem *src = m_nodes.value(s.id);
        if (!src) continue;
        if (s.type.compare("Choice", Qt::CaseInsensitive) == 0
                && !s.branches.isEmpty()) {
            for (const auto &b : s.branches) {
                BpmnNodeItem *dst = m_nodes.value(b.targetId);
                if (!dst) continue;
                auto *e = new BpmnEdgeItem(src, dst, b.label.isEmpty()
                                                          ? b.returnValue
                                                          : b.label);
                m_scene->addItem(e);
                m_edges.append(e);
            }
        } else if (!s.nextId.isEmpty()) {
            BpmnNodeItem *dst = m_nodes.value(s.nextId);
            if (!dst) continue;
            auto *e = new BpmnEdgeItem(src, dst, QString());
            m_scene->addItem(e);
            m_edges.append(e);
        }
    }
    redrawEdges();
    QRectF bounds = m_scene->itemsBoundingRect().adjusted(-40, -40, 40, 40);
    m_scene->setSceneRect(bounds.isEmpty() ? QRectF(0,0,800,400) : bounds);
}

void BpmnCanvas::redrawEdges() {
    for (auto *e : m_edges) e->update();
}

void BpmnCanvas::syncLayoutToModel() {
    if (!m_process) return;
    for (auto &s : m_process->steps()) {
        if (auto *item = m_nodes.value(s.id))
            s.pos = item->pos();
    }
}

BpmnNodeItem *BpmnCanvas::nodeFor(const QString &id) const {
    return m_nodes.value(id);
}

void BpmnCanvas::onSelectionChanged() {
    auto sel = m_scene->selectedItems();
    if (sel.isEmpty()) { emit stepSelected(QString()); return; }
    if (auto *n = qgraphicsitem_cast<BpmnNodeItem*>(sel.first())) {
        emit stepSelected(n->stepId());
    }
}

void BpmnCanvas::contextMenuEvent(QContextMenuEvent *e) {
    if (!m_process) return;
    QPoint vp = e->pos();
    QPointF scenePt = mapToScene(vp);
    QGraphicsItem *under = scene()->itemAt(scenePt, QTransform());

    QMenu menu(this);
    auto *addServer = menu.addAction("Add Server task here");
    auto *addChoice = menu.addAction("Add Choice gateway here");
    auto *addHuman  = menu.addAction("Add Human task here");
    auto *addFinal  = menu.addAction("Add End event here");
    QAction *connectAct = nullptr, *delAct = nullptr;
    BpmnNodeItem *src = qgraphicsitem_cast<BpmnNodeItem*>(under);
    if (src) {
        menu.addSeparator();
        connectAct = menu.addAction("Connect to existing step…");
        delAct     = menu.addAction("Delete step");
    }
    QAction *picked = menu.exec(e->globalPos());
    if (!picked) return;

    auto append = [&](const QString &type){
        StepSpec s;
        s.id   = QString("%1_%2").arg(type).arg(m_process->steps().size()+1);
        s.type = type;
        s.pos  = scenePt;
        s.size = (type == "Choice") ? QSizeF(kGwSize, kGwSize)
               : (type == "Final" ? QSizeF(kEvSize, kEvSize)
               :                    QSizeF(kStepW, kStepH));
        m_process->steps().append(s);
        emit modelChanged();
        rebuild();
    };

    if (picked == addServer) append("Server");
    else if (picked == addChoice) append("Choice");
    else if (picked == addHuman)  append("HumanTask");
    else if (picked == addFinal)  append("Final");
    else if (picked == connectAct && src) {
        QStringList ids;
        for (const auto &s : m_process->steps())
            if (s.id != src->stepId()) ids << s.id;
        if (ids.isEmpty()) return;
        bool ok = false;
        QString tgt = QInputDialog::getItem(this, "Connect",
            QString("Connect '%1' to:").arg(src->stepId()), ids, 0, false, &ok);
        if (!ok || tgt.isEmpty()) return;
        for (auto &s : m_process->steps()) {
            if (s.id != src->stepId()) continue;
            if (s.type.compare("Choice", Qt::CaseInsensitive) == 0) {
                StepSpec::Branch b;
                bool bok = false;
                QString rv = QInputDialog::getText(this, "Branch return value",
                    QString("Body returns this value to take this branch:"),
                    QLineEdit::Normal, tgt, &bok);
                if (!bok) return;
                b.returnValue = rv.trimmed().isEmpty() ? tgt : rv.trimmed();
                b.targetId    = tgt;
                b.label       = b.returnValue;
                s.branches.append(b);
            } else {
                s.nextId = tgt;
            }
            break;
        }
        emit modelChanged();
        rebuild();
    }
    else if (picked == delAct && src) {
        QString id = src->stepId();
        auto &steps = m_process->steps();
        for (int i = 0; i < steps.size(); ++i) {
            if (steps[i].id == id) { steps.remove(i); break; }
        }
        for (auto &s : steps) {
            if (s.nextId == id) s.nextId.clear();
            for (int i = s.branches.size()-1; i >= 0; --i)
                if (s.branches[i].targetId == id) s.branches.remove(i);
        }
        emit modelChanged();
        rebuild();
    }
}

} // namespace nx
