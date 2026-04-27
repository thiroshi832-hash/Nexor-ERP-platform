#include "CentralStack.h"

#include "welcome/WelcomePage.h"
#include "editor/CodeEditor.h"
#include "designer/DesignerView.h"
#include "designer/FormCanvas.h"

#include <QLabel>
#include <QVBoxLayout>

namespace {
QWidget *placeholder(const QString &title, const QString &subtitle) {
    auto *w = new QWidget;
    w->setStyleSheet("background:#13151b;");
    auto *col = new QVBoxLayout(w);
    col->setAlignment(Qt::AlignCenter);
    auto *t = new QLabel(title);
    t->setStyleSheet("color:#5b8cff; font-size:24px; font-weight:300; letter-spacing:3px;");
    t->setAlignment(Qt::AlignCenter);
    auto *s = new QLabel(subtitle);
    s->setStyleSheet("color:#6b7280; font-size:13px;");
    s->setAlignment(Qt::AlignCenter);
    s->setWordWrap(true);
    col->addWidget(t);
    col->addSpacing(10);
    col->addWidget(s);
    return w;
}
}

CentralStack::CentralStack(QWidget *parent) : QStackedWidget(parent) {
    m_welcome      = new WelcomePage;
    m_editor       = new CodeEditor;
    m_designerView = new DesignerView;

    addWidget(m_welcome);        // PageWelcome
    addWidget(m_editor);         // PageEditor
    addWidget(m_designerView);   // PageDesigner
    addWidget(placeholder("BUILD",
        "Compilation output will appear here.\n"
        "Compiler lands in feature/language-compiler."));
    addWidget(placeholder("DEBUG",
        "Breakpoint, step and watch panels will live here.\n"
        "Debugger lands in feature/debugger."));
}

FormCanvas *CentralStack::formCanvas() const {
    return m_designerView ? m_designerView->formCanvas() : nullptr;
}
