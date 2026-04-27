#include "FormRunner.h"
#include "designer/WidgetFactory.h"

#include <QDialog>
#include <QFile>
#include <QXmlStreamReader>
#include <QWidget>

bool FormRunner::runForm(const QString &filePath, QWidget *parent) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;

    auto *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    int formW = 640, formH = 480;
    QString title;

    QXmlStreamReader r(&f);
    bool inWidget = false;
    QString cType, cName, cText;
    int cx = 0, cy = 0, cw = 0, ch = 0;

    while (!r.atEnd()) {
        r.readNext();
        if (r.isStartElement()) {
            const QStringRef n = r.name();
            if (n == "Geometry") {
                const auto a = r.attributes();
                if (a.hasAttribute("width"))  formW = a.value("width").toInt();
                if (a.hasAttribute("height")) formH = a.value("height").toInt();
            } else if (n == "Title") {
                title = r.readElementText();
            } else if (n == "Widget") {
                inWidget = true; cText.clear();
                const auto a = r.attributes();
                cType = a.value("type").toString();
                cName = a.value("name").toString();
                cx = a.value("x").toInt();
                cy = a.value("y").toInt();
                cw = a.value("width").toInt();
                ch = a.value("height").toInt();
            } else if (n == "Property" && inWidget) {
                QString pn = r.attributes().value("name").toString();
                QString pv = r.readElementText();
                if (pn == "text") cText = pv;
            }
        } else if (r.isEndElement()) {
            if (r.name() == "Widget" && inWidget) {
                if (QWidget *w = WidgetFactory::create(cType, dlg)) {
                    if (cw < 4) cw = WidgetFactory::defaultSize(cType).width();
                    if (ch < 4) ch = WidgetFactory::defaultSize(cType).height();
                    w->setGeometry(cx, cy, cw, ch);
                    if (!cText.isEmpty())
                        WidgetFactory::applyProperty(w, "text", cText);
                    if (!cName.isEmpty())
                        w->setObjectName(cName);
                }
                inWidget = false;
            }
        }
    }

    dlg->setWindowTitle(title);
    dlg->resize(formW, formH);
    dlg->show();
    return !r.hasError();
}
