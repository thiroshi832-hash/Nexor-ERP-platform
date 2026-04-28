#include "FormRunner.h"
#include "designer/WidgetFactory.h"
#include "language/NexorRuntime.h"
#include "language/EntityStore.h"
#include "runtime/WidgetValue.h"
#include "project/Project.h"

#include <QDialog>
#include <QFile>
#include <QXmlStreamReader>
#include <QWidget>
#include <QAbstractButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QSlider>
#include <QFileInfo>
#include <QTimer>
#include <QPointer>

namespace {

// Wires Qt signal → interpreter call for each designed widget that supports
// at least one of the standard events (Click, Change).
void hookEvents(QWidget *w,
                const QString &name,
                std::shared_ptr<nx::NexorRuntime> rt) {
    auto fire = [rt, name](const QString &event, const QVector<nx::Value> &args = {}) {
        QString sub = name + "_" + event;
        if (rt->hasSub(sub)) rt->call(sub, args);
    };

    if (auto *b = qobject_cast<QAbstractButton*>(w)) {
        QObject::connect(b, &QAbstractButton::clicked, b, [fire]{ fire("Click"); });
        if (qobject_cast<QCheckBox*>(b) || qobject_cast<QRadioButton*>(b)) {
            QObject::connect(b, &QAbstractButton::toggled, b,
                             [fire](bool){ fire("Change"); });
        }
        return;
    }
    if (auto *e = qobject_cast<QLineEdit*>(w)) {
        QObject::connect(e, &QLineEdit::textChanged, e, [fire](const QString&){ fire("Change"); });
        QObject::connect(e, &QLineEdit::returnPressed, e, [fire]{ fire("Click"); });
        return;
    }
    if (auto *e = qobject_cast<QPlainTextEdit*>(w)) {
        QObject::connect(e, &QPlainTextEdit::textChanged, e, [fire]{ fire("Change"); });
        return;
    }
    if (auto *c = qobject_cast<QComboBox*>(w)) {
        QObject::connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged),
                         c, [fire](int){ fire("Change"); });
        return;
    }
    if (auto *s = qobject_cast<QSpinBox*>(w)) {
        QObject::connect(s, QOverload<int>::of(&QSpinBox::valueChanged),
                         s, [fire](int){ fire("Change"); });
        return;
    }
    if (auto *s = qobject_cast<QSlider*>(w)) {
        QObject::connect(s, &QSlider::valueChanged, s, [fire](int){ fire("Change"); });
        return;
    }
}

// Reads the form file and yields back the four pieces FormRunner needs:
// title, formW, formH, code, and the list of widgets to instantiate.
struct FormSpec {
    QString title;
    QString dataSource;                  // empty == no entity binding
    int     w { 640 }, h { 480 };
    QString code;
    struct WidgetEntry {
        QString type, name, text, binding;
        int x = 0, y = 0, w = 0, h = 0;
    };
    QVector<WidgetEntry> widgets;
};

bool readFormFile(const QString &path, FormSpec &spec) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QXmlStreamReader r(&f);
    bool inWidget = false;
    FormSpec::WidgetEntry cur;
    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement()) {
            const auto n = r.name();
            if (n == "Form") {
                spec.dataSource = r.attributes().value("dataSource").toString();
            } else if (n == "Geometry") {
                const auto a = r.attributes();
                if (a.hasAttribute("width"))  spec.w = a.value("width").toInt();
                if (a.hasAttribute("height")) spec.h = a.value("height").toInt();
            } else if (n == "Title") {
                spec.title = r.readElementText();
            } else if (n == "Code") {
                spec.code = r.readElementText();
            } else if (n == "Widget") {
                inWidget = true; cur = {};
                const auto a = r.attributes();
                cur.type    = a.value("type").toString();
                cur.name    = a.value("name").toString();
                cur.binding = a.value("binding").toString();
                cur.x    = a.value("x").toInt();
                cur.y    = a.value("y").toInt();
                cur.w    = a.value("width").toInt();
                cur.h    = a.value("height").toInt();
            } else if (n == "Property" && inWidget) {
                QString pn = r.attributes().value("name").toString();
                QString pv = r.readElementText();
                if (pn == "text") cur.text = pv;
            }
        } else if (r.isEndElement() && r.name() == "Widget" && inWidget) {
            spec.widgets.append(cur);
            inWidget = false;
        }
    }
    return !r.hasError();
}

} // namespace

// FormContext lives for the lifetime of the running form.  It carries the
// data binding + a pointer to the currently-bound entity so that Form.Save /
// Form.Load can pull values into / push values out of the right widgets.
namespace {
struct FormContext {
    QString                                 dataSource;
    QHash<QString, QString>                 bindings;     // widgetName → fieldName  (lowercase keys)
    QHash<QString, QPointer<QWidget>>       widgets;      // widgetName → widget     (lowercase keys)
    nx::EntityStore                        *store    { nullptr };
    std::shared_ptr<nx::Entity>             current;

    QPointer<QWidget> widgetFor(const QString &name) const {
        return widgets.value(name.toLower());
    }

    // Pull current entity field values into bound widgets.
    void pushEntityToWidgets() {
        if (!current) return;
        for (auto it = bindings.begin(); it != bindings.end(); ++it) {
            QPointer<QWidget> w = widgetFor(it.key());
            if (!w) continue;
            nx::WidgetValue::write(w.data(), current->get(it.value()));
        }
    }
    // Pull bound widget values back into the entity.
    void pullWidgetsToEntity() {
        if (!current) return;
        for (auto it = bindings.begin(); it != bindings.end(); ++it) {
            QPointer<QWidget> w = widgetFor(it.key());
            if (!w) continue;
            current->set(it.value(), nx::WidgetValue::read(w.data()));
        }
    }
};
} // namespace

bool FormRunner::runForm(const QString &filePath,
                         QWidget *parent,
                         OutputFn out,
                         OutputFn err,
                         const Project *project) {
    FormSpec spec;
    if (!readFormFile(filePath, spec)) return false;

    auto *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(spec.title);
    dlg->resize(spec.w, spec.h);

    auto rt = std::make_shared<nx::NexorRuntime>();
    if (out) rt->setOutput(out);
    if (err) rt->setError(err);

    // Open the project store + register sheets so the running form can use
    // entity types in script (Customer.Find(1) etc.).
    if (project) rt->registerProjectSheets(project);

    auto ctx       = std::make_shared<FormContext>();
    ctx->dataSource = spec.dataSource;
    ctx->store      = rt->interpreter()->entityStore();

    // Instantiate widgets, set objectName, record bindings, hook events.
    for (const auto &we : spec.widgets) {
        if (QWidget *w = WidgetFactory::create(we.type, dlg)) {
            int wW = we.w > 0 ? we.w : WidgetFactory::defaultSize(we.type).width();
            int wH = we.h > 0 ? we.h : WidgetFactory::defaultSize(we.type).height();
            w->setGeometry(we.x, we.y, wW, wH);
            if (!we.text.isEmpty())
                WidgetFactory::applyProperty(w, "text", we.text);
            if (!we.name.isEmpty())
                w->setObjectName(we.name);
            ctx->widgets.insert(we.name.toLower(), QPointer<QWidget>(w));
            if (!we.binding.isEmpty())
                ctx->bindings.insert(we.name.toLower(), we.binding);
            hookEvents(w, we.name, rt);
        }
    }

    // Install Form bridge — Form.Save() / Load(id) / New() / Delete() / Current.
    auto interp = rt->interpreter();
    interp->setFormHandler([ctx](const QString &method,
                                 const QVector<nx::Value> &args) -> nx::Value {
        QString lo = method.toLower();
        if (ctx->dataSource.isEmpty() || !ctx->store) return nx::Value();
        nx::EntityTable *table = ctx->store->table(ctx->dataSource);
        if (!table) return nx::Value();

        if (lo == "new") {
            ctx->current = table->create();
            ctx->pushEntityToWidgets();
            return nx::Value::object(ctx->current, "Entity");
        }
        if (lo == "load") {
            qint64 id = args.isEmpty() ? 0 : args.first().toLong();
            ctx->current = table->find(id);
            ctx->pushEntityToWidgets();
            return ctx->current ? nx::Value::object(ctx->current, "Entity") : nx::Value();
        }
        if (lo == "save") {
            if (!ctx->current) ctx->current = table->create();
            ctx->pullWidgetsToEntity();
            return nx::Value::boolean(table->save(ctx->current));
        }
        if (lo == "delete") {
            if (!ctx->current || !ctx->current->isPersisted()) return nx::Value::boolean(false);
            bool ok = table->remove(ctx->current->id());
            if (ok) ctx->current.reset();
            return nx::Value::boolean(ok);
        }
        return nx::Value();
    });
    interp->setFormReader([ctx](const QString &prop) -> nx::Value {
        QString lo = prop.toLower();
        if (lo == "current")
            return ctx->current ? nx::Value::object(ctx->current, "Entity") : nx::Value();
        if (lo == "datasource")
            return nx::Value::text(ctx->dataSource);
        // Otherwise, treat as a widget name and return the widget's primary value.
        QPointer<QWidget> w = ctx->widgetFor(prop);
        if (w) return nx::WidgetValue::read(w.data());
        return nx::Value();
    });
    interp->setFormWriter([ctx](const QString &prop, const nx::Value &v) {
        // Form.<widgetName> = value  → write to the widget's primary value.
        QPointer<QWidget> w = ctx->widgetFor(prop);
        if (w) nx::WidgetValue::write(w.data(), v);
    });

    QString unitName = QFileInfo(filePath).fileName();
    if (!rt->compile(spec.code, unitName) && err) {
        err(QString("compile error in %1: %2").arg(unitName, rt->lastError()));
    }

    // Form_Unload on dialog destruction; Form_Load right after first paint.
    QObject::connect(dlg, &QDialog::destroyed, [rt]{
        if (rt->hasSub("Form_Unload")) rt->call("Form_Unload");
    });
    dlg->show();
    QTimer::singleShot(0, dlg, [rt]{
        if (rt->hasSub("Form_Load")) rt->call("Form_Load");
    });
    return true;
}
