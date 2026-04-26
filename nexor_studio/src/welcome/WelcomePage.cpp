#include "WelcomePage.h"

#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QToolButton>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QFrame>

namespace {

QListWidget *makeCardList(QWidget *parent) {
    auto *list = new QListWidget(parent);
    list->setFrameShape(QFrame::NoFrame);
    list->setSpacing(6);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setStyleSheet(R"(
        QListWidget {
            background:#13151b; color:#dce1e7; border:none;
            font-size:13px; padding:4px 4px 4px 0;
        }
        QListWidget::item {
            background:#1b1d23; border:1px solid #262932;
            border-radius:6px; padding:14px 16px; margin:0;
        }
        QListWidget::item:hover    { border-color:#5b8cff; background:#1f2230; }
        QListWidget::item:selected { background:#1e3a5f; border-color:#5b8cff;
                                     color:#ffffff; }
    )");
    return list;
}

} // namespace

WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent)
    , m_newBtn(nullptr), m_openBtn(nullptr)
    , m_tabGroup(nullptr)
    , m_tabProjects(nullptr), m_tabExamples(nullptr), m_tabTutorials(nullptr)
    , m_contentStack(nullptr)
    , m_recentList(nullptr), m_exampleList(nullptr), m_tutorialList(nullptr)
{
    setObjectName("welcomePage");
    setStyleSheet(R"(
        QWidget#welcomePage  { background:#13151b; }
        QWidget#sideBar      { background:#0f1116; border-right:1px solid #1e2030; }
        QWidget#footer       { background:#0d0e12; border-top:1px solid #1e2030; }
        QLabel#brand         { color:#5b8cff; font-size:24px; font-weight:300;
                               letter-spacing:5px; }
        QLabel#tagline       { color:#6b7280; font-size:11px; letter-spacing:2px; }
        QLabel#sectionTitle  { color:#dce1e7; font-size:22px; font-weight:300;
                               letter-spacing:1px; }
        QLabel#sectionLabel  { color:#6b7280; font-size:10px; font-weight:600;
                               letter-spacing:2px; }
        QToolButton#bigBtn {
            background:#1e3a5f; color:#dce1e7;
            border:1px solid #2a4a72; border-radius:6px;
            padding:10px 14px; font-size:13px; font-weight:600;
            text-align:left;
        }
        QToolButton#bigBtn:hover    { background:#26477a; border-color:#5b8cff; }
        QToolButton#bigBtn:pressed  { background:#1a3252; }
        QToolButton#tabBtn {
            background:transparent; color:#8a95a3;
            border:none; padding:9px 18px; text-align:left;
            font-size:13px; font-weight:600; letter-spacing:1px;
        }
        QToolButton#tabBtn:hover    { color:#dce1e7; background:#181a22; }
        QToolButton#tabBtn:checked  { color:#5b8cff; background:#181a22;
                                      border-left:3px solid #5b8cff;
                                      padding-left:15px; }
        QToolButton#footerLink {
            background:transparent; color:#8a95a3; border:none;
            padding:8px 14px; font-size:12px; font-weight:500;
        }
        QToolButton#footerLink:hover { color:#5b8cff; }
    )");

    // Outer layout: vertical = [content row | footer]
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // Content row: [side bar | content stack]
    auto *contentRow = new QHBoxLayout;
    contentRow->setContentsMargins(0, 0, 0, 0);
    contentRow->setSpacing(0);

    contentRow->addWidget(buildSideBar(), 0);

    m_contentStack = new QStackedWidget;
    m_contentStack->addWidget(buildProjectsView());   // 0
    m_contentStack->addWidget(buildExamplesView());   // 1
    m_contentStack->addWidget(buildTutorialsView());  // 2
    contentRow->addWidget(m_contentStack, 1);

    outer->addLayout(contentRow, 1);

    outer->addWidget(buildFooter(), 0);

    // Default sub-tab: Projects
    m_tabProjects->setChecked(true);
    onSubTabClicked(TabProjects);
}

QWidget *WelcomePage::buildSideBar() {
    auto *side = new QWidget;
    side->setObjectName("sideBar");
    side->setFixedWidth(240);

    auto *col = new QVBoxLayout(side);
    col->setContentsMargins(20, 28, 20, 20);
    col->setSpacing(14);

    // Brand
    auto *brand   = new QLabel("NEXOR");           brand->setObjectName("brand");
    auto *tagline = new QLabel("STUDIO  ·  0.1");  tagline->setObjectName("tagline");
    col->addWidget(brand);
    col->addWidget(tagline);

    col->addSpacing(20);

    // Action buttons
    m_newBtn = new QToolButton;
    m_newBtn->setObjectName("bigBtn");
    m_newBtn->setText("  +    New Project");
    m_newBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_newBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_openBtn = new QToolButton;
    m_openBtn->setObjectName("bigBtn");
    m_openBtn->setText("  ⇪    Open Project...");
    m_openBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_openBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    col->addWidget(m_newBtn);
    col->addWidget(m_openBtn);

    col->addSpacing(28);

    // Sub-tab selectors
    auto *navLbl = new QLabel("BROWSE"); navLbl->setObjectName("sectionLabel");
    col->addWidget(navLbl);

    m_tabGroup = new QButtonGroup(this);
    m_tabGroup->setExclusive(true);

    auto makeTab = [&](const QString &text, int id) -> QToolButton* {
        auto *b = new QToolButton;
        b->setObjectName("tabBtn");
        b->setText(text);
        b->setCheckable(true);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_tabGroup->addButton(b, id);
        return b;
    };

    m_tabProjects  = makeTab("Projects",  TabProjects);
    m_tabExamples  = makeTab("Examples",  TabExamples);
    m_tabTutorials = makeTab("Tutorials", TabTutorials);

    // Use a tight container so the stripe-on-checked aligns to the side bar
    auto *tabsBox = new QVBoxLayout;
    tabsBox->setContentsMargins(-20, 0, -20, 0); // bleed to side-bar edges
    tabsBox->setSpacing(2);
    tabsBox->addWidget(m_tabProjects);
    tabsBox->addWidget(m_tabExamples);
    tabsBox->addWidget(m_tabTutorials);
    col->addLayout(tabsBox);

    col->addStretch();

    connect(m_newBtn,  &QToolButton::clicked, this, &WelcomePage::newProjectRequested);
    connect(m_openBtn, &QToolButton::clicked, this, &WelcomePage::openProjectRequested);
    connect(m_tabGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &WelcomePage::onSubTabClicked);

    return side;
}

QWidget *WelcomePage::buildProjectsView() {
    auto *page = new QWidget;
    auto *col = new QVBoxLayout(page);
    col->setContentsMargins(40, 36, 40, 24);
    col->setSpacing(16);

    auto *title = new QLabel("Recent Projects"); title->setObjectName("sectionTitle");
    col->addWidget(title);

    m_recentList = makeCardList(page);
    col->addWidget(m_recentList, 1);

    connect(m_recentList, &QListWidget::itemActivated, this, [this](QListWidgetItem *it){
        if (it && (it->flags() & Qt::ItemIsSelectable))
            emit recentProjectActivated(it->data(Qt::UserRole).toString());
    });
    return page;
}

QWidget *WelcomePage::buildExamplesView() {
    auto *page = new QWidget;
    auto *col = new QVBoxLayout(page);
    col->setContentsMargins(40, 36, 40, 24);
    col->setSpacing(16);

    auto *title = new QLabel("Examples"); title->setObjectName("sectionTitle");
    col->addWidget(title);

    m_exampleList = makeCardList(page);
    auto add = [this](const QString &title, const QString &subtitle) {
        auto *it = new QListWidgetItem(QString("%1\n   %2").arg(title, subtitle));
        it->setData(Qt::UserRole, title);
        m_exampleList->addItem(it);
    };
    add("Hello World",       "Single-form atomic activity that prints to console");
    add("Calculator",        "Two-input form with code-behind doing arithmetic");
    add("Account Book",      "Multi-form activity with shared global variables");
    add("Web Service Client","Demonstrates Resources and HTTP calls");
    col->addWidget(m_exampleList, 1);

    connect(m_exampleList, &QListWidget::itemActivated, this, [this](QListWidgetItem *it){
        if (it) emit exampleActivated(it->data(Qt::UserRole).toString());
    });
    return page;
}

QWidget *WelcomePage::buildTutorialsView() {
    auto *page = new QWidget;
    auto *col = new QVBoxLayout(page);
    col->setContentsMargins(40, 36, 40, 24);
    col->setSpacing(16);

    auto *title = new QLabel("Tutorials"); title->setObjectName("sectionTitle");
    col->addWidget(title);

    m_tutorialList = makeCardList(page);
    auto add = [this](const QString &t, const QString &s) {
        auto *it = new QListWidgetItem(QString("%1\n   %2").arg(t, s));
        m_tutorialList->addItem(it);
    };
    add("Your First Activity",   "Create a new project and run an activity");
    add("Designing Forms",       "Drag-and-drop UI in the form designer");
    add("The Nexor Language",    "Subs, variables, and global state");
    add("Connecting to Core",    "Talk to the Nexor backend from Flux");
    col->addWidget(m_tutorialList, 1);

    return page;
}

QWidget *WelcomePage::buildFooter() {
    auto *footer = new QWidget;
    footer->setObjectName("footer");
    footer->setFixedHeight(40);
    auto *row = new QHBoxLayout(footer);
    row->setContentsMargins(20, 0, 20, 0);
    row->setSpacing(2);

    auto add = [&](const QString &label){
        auto *b = new QToolButton;
        b->setObjectName("footerLink");
        b->setText(label);
        b->setCursor(Qt::PointingHandCursor);
        row->addWidget(b);
    };
    add("Documentation");
    add("·");
    add("Examples");
    add("·");
    add("GitHub");
    row->addStretch();
    auto *ver = new QLabel("Nexor Studio 0.1.0");
    ver->setStyleSheet("color:#4a5060; font-size:11px;");
    row->addWidget(ver);
    return footer;
}

void WelcomePage::onSubTabClicked(int tab) {
    if (m_contentStack) m_contentStack->setCurrentIndex(tab);
}

void WelcomePage::setRecentProjects(const QStringList &paths) {
    if (!m_recentList) return;
    m_recentList->clear();
    if (paths.isEmpty()) {
        auto *empty = new QListWidgetItem(
            "No recent projects\n   "
            "Click \"+ New Project\" on the left to get started.");
        empty->setFlags(Qt::ItemIsEnabled);   // not selectable
        m_recentList->addItem(empty);
        return;
    }
    for (const QString &p : paths) {
        QFileInfo fi(p);
        auto *it = new QListWidgetItem(QString("%1\n   %2")
                                         .arg(fi.completeBaseName(), fi.absoluteFilePath()));
        it->setData(Qt::UserRole, fi.absoluteFilePath());
        m_recentList->addItem(it);
    }
}
