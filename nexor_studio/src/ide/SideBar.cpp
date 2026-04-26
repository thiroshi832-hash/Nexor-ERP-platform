#include "SideBar.h"

#include <QToolButton>
#include <QButtonGroup>
#include <QVBoxLayout>

SideBar::SideBar(QWidget *parent) : QWidget(parent), m_group(new QButtonGroup(this)) {
    setFixedWidth(58);
    setStyleSheet(R"(
        SideBar { background:#0d0e12; border-right:1px solid #1e2030; }
        QToolButton {
            background:transparent; color:#8a95a3; border:none;
            border-left:3px solid transparent; padding:14px 4px;
            font-family:"Segoe UI"; font-size:10px; font-weight:600;
            letter-spacing:1px;
        }
        QToolButton:hover     { color:#dce1e7; background:#181a22; }
        QToolButton:checked   { color:#5b8cff; background:#181a22;
                                border-left-color:#5b8cff; }
    )");

    auto *col = new QVBoxLayout(this);
    col->setContentsMargins(0, 8, 0, 0);
    col->setSpacing(0);

    col->addWidget(makeBtn("PROJ",   ModeProjects));
    col->addWidget(makeBtn("EDIT",   ModeEdit));
    col->addWidget(makeBtn("DESIGN", ModeDesigner));
    col->addWidget(makeBtn("BUILD",  ModeBuild));
    col->addWidget(makeBtn("DEBUG",  ModeDebug));
    col->addStretch();

    m_group->setExclusive(true);
    if (auto *first = qobject_cast<QToolButton*>(m_group->button(ModeProjects)))
        first->setChecked(true);

    connect(m_group, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &SideBar::modeChanged);
}

QToolButton *SideBar::makeBtn(const QString &text, int id) {
    auto *b = new QToolButton(this);
    b->setText(text);
    b->setCheckable(true);
    b->setToolButtonStyle(Qt::ToolButtonTextOnly);
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_group->addButton(b, id);
    return b;
}

void SideBar::setMode(Mode m) {
    if (auto *b = qobject_cast<QToolButton*>(m_group->button(m))) b->setChecked(true);
}
