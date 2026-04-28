#include "FormRunner.h"
#include "designer/WidgetFactory.h"
#include "language/NexorRuntime.h"

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
    int     w { 640 }, h { 480 };
    QString code;
    struct WidgetEntry {
        QString type, name, text;
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
            if (n == "Geometry") {
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
                cur.type = a.value("type").toString();
                cur.name = a.value("name").toString();
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

bool FormRunner::runForm(const QString &filePath,
                         QWidget *parent,
                         OutputFn out,
                         OutputFn err) {
    FormSpec spec;
    if (!readFormFile(filePath, spec)) return false;

    auto *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(spec.title);
    dlg->resize(spec.w, spec.h);

    // Compile the form's <Code> block once.
    auto rt = std::make_shared<nx::NexorRuntime>();
    if (out) rt->setOutput(out);
    if (err) rt->setError(err);

    QString unitName = QFileInfo(filePath).fileName();
    if (!rt->compile(spec.code, unitName) && err) {
        err(QString("compile error in %1: %2").arg(unitName, rt->lastError()));
    }

    // Instantiate widgets, set their objectName to the designed name (so user
    // code can find them via Qt object lookup if we wire that later), and
    // hook standard events to interpreter calls.
    for (const auto &we : spec.widgets) {
        if (QWidget *w = WidgetFactory::create(we.type, dlg)) {
            int wW = we.w > 0 ? we.w : WidgetFactory::defaultSize(we.type).width();
            int wH = we.h > 0 ? we.h : WidgetFactory::defaultSize(we.type).height();
            w->setGeometry(we.x, we.y, wW, wH);
            if (!we.text.isEmpty())
                WidgetFactory::applyProperty(w, "text", we.text);
            if (!we.name.isEmpty())
                w->setObjectName(we.name);
            hookEvents(w, we.name, rt);
        }
    }

    // Form_Load fires right after the dialog becomes visible.
    QObject::connect(dlg, &QDialog::destroyed, [rt]{
        if (rt->hasSub("Form_Unload")) rt->call("Form_Unload");
    });
    dlg->show();
    QTimer::singleShot(0, dlg, [rt]{
        if (rt->hasSub("Form_Load")) rt->call("Form_Load");
    });
    return true;
}
