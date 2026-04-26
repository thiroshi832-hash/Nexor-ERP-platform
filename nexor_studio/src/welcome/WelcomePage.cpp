#include "WelcomePage.h"

#include <QLabel>
#include <QStackedWidget>
#include <QToolButton>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QAbstractButton>
#include <QFontMetrics>

namespace {

// =============================================================================
// WelcomeRow — one numbered list item (Sessions or Projects).
//
//   1   ▶   default
//             (last session)
//
// Inherits QAbstractButton so we get clicked() and hover/press states for free.
// =============================================================================
class WelcomeRow : public QAbstractButton {
public:
    enum Icon { IconPlay, IconFolder };

    WelcomeRow(int number, Icon icon, const QString &title,
               const QString &subtitle, QWidget *parent = nullptr)
        : QAbstractButton(parent)
        , m_number(number)
        , m_icon(icon)
        , m_title(title)
        , m_subtitle(subtitle) {
        setCursor(Qt::PointingHandCursor);
        setFixedHeight(subtitle.isEmpty() ? 28 : 44);
        setAttribute(Qt::WA_Hover, true);
    }

    QSize sizeHint() const override {
        return QSize(320, height());
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Hover background
        if (underMouse()) {
            p.fillRect(rect(), QColor(0xff, 0xff, 0xff, 18));
        }

        // Number column (left)
        QRect numRect(8, 0, 22, height());
        p.setPen(QColor(0x9a, 0x9a, 0x9a));
        QFont nf = p.font();
        nf.setPointSize(9);
        p.setFont(nf);
        p.drawText(numRect, Qt::AlignTop | Qt::AlignRight,
                   QString::number(m_number));

        // Icon column
        QRect iconRect(36, 4, 18, 18);
        drawIcon(p, iconRect, QColor(0xb0, 0xb0, 0xb0));

        // Title (green)
        QRect titleRect(60, 2, width() - 64, 20);
        QFont tf = p.font();
        tf.setPointSize(10);
        p.setFont(tf);
        p.setPen(underMouse() ? QColor(0x6a, 0xe0, 0x7a)
                              : QColor(0x41, 0xcd, 0x52));
        p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_title);

        // Subtitle (gray, smaller)
        if (!m_subtitle.isEmpty()) {
            QRect subRect(60, 22, width() - 64, 18);
            QFont sf = p.font();
            sf.setPointSize(8);
            p.setFont(sf);
            p.setPen(QColor(0x8a, 0x8a, 0x8a));
            QFontMetrics fm(sf);
            QString elided = fm.elidedText(m_subtitle, Qt::ElideMiddle,
                                           subRect.width());
            p.drawText(subRect, Qt::AlignLeft | Qt::AlignVCenter, elided);
        }
    }

    void enterEvent(QEvent *) override { update(); }
    void leaveEvent(QEvent *) override { update(); }

private:
    void drawIcon(QPainter &p, const QRect &r, const QColor &c) {
        QPen pen(c, 1.4);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        if (m_icon == IconPlay) {
            QPainterPath tri;
            tri.moveTo(r.left()  + 4, r.top()    + 2);
            tri.lineTo(r.right() - 3, r.center().y());
            tri.lineTo(r.left()  + 4, r.bottom() - 2);
            tri.closeSubpath();
            p.fillPath(tri, c);
        } else { // IconFolder
            QPainterPath f;
            f.moveTo(r.left(),     r.top() + 5);
            f.lineTo(r.left() + 6, r.top() + 5);
            f.lineTo(r.left() + 8, r.top() + 7);
            f.lineTo(r.right(),    r.top() + 7);
            f.lineTo(r.right(),    r.bottom() - 1);
            f.lineTo(r.left(),     r.bottom() - 1);
            f.closeSubpath();
            p.drawPath(f);
        }
    }

    int     m_number;
    Icon    m_icon;
    QString m_title;
    QString m_subtitle;
};

// =============================================================================
// Helpers for action buttons (Manage / + New / ▥ Open) and small bottom links.
// =============================================================================
QToolButton *makeActionBtn(const QString &iconText, const QString &label,
                           QWidget *parent) {
    auto *b = new QToolButton(parent);
    b->setObjectName("actionBtn");
    b->setText(QString("  %1   %2  ").arg(iconText, label));
    b->setToolButtonStyle(Qt::ToolButtonTextOnly);
    b->setCursor(Qt::PointingHandCursor);
    b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return b;
}

QToolButton *makeFooterLink(const QString &iconText, const QString &label,
                            QWidget *parent) {
    auto *b = new QToolButton(parent);
    b->setObjectName("footerLink");
    b->setText(QString("%1   %2").arg(iconText, label));
    b->setToolButtonStyle(Qt::ToolButtonTextOnly);
    b->setCursor(Qt::PointingHandCursor);
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return b;
}

QFrame *vline() {
    auto *f = new QFrame;
    f->setFrameShape(QFrame::VLine);
    f->setStyleSheet("color:#1e2030;");
    return f;
}

} // namespace


// =============================================================================
// WelcomePage
// =============================================================================
WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent)
    , m_newBtn(nullptr), m_openBtn(nullptr)
    , m_manageSessionsBtn(nullptr), m_getStartedBtn(nullptr)
    , m_subTabGroup(nullptr)
    , m_tabProjects(nullptr), m_tabExamples(nullptr), m_tabTutorials(nullptr)
    , m_contentStack(nullptr)
    , m_recentRowsLayout(nullptr)
{
    setObjectName("welcomePage");
    setStyleSheet(R"(
        QWidget#welcomePage    { background:#4d4d4d; }
        QWidget#welcomeSidebar { background:#3c3c3c; }
        QWidget#welcomeContent { background:#4d4d4d; }
        QWidget#welcomeFooter  { background:#3c3c3c; border-top:1px solid #2a2a2a; }

        QLabel#sectionTitle    { color:#dcdcdc; font-size:18px; font-weight:400; }
        QLabel#promoTitle      { color:#dcdcdc; font-size:14px; font-weight:600; }
        QLabel#promoBody       { color:#a8a8a8; font-size:11px; }
        QLabel#columnHeader    { color:#dcdcdc; font-size:13px; font-weight:600; }

        /* Big Projects/Examples/Tutorials tab buttons */
        QToolButton#subTab {
            background:#4d4d4d; color:#dcdcdc; border:none;
            padding:11px 18px; font-size:13px; font-weight:500;
            text-align:left;
        }
        QToolButton#subTab:hover    { background:#555; }
        QToolButton#subTab:checked  { background:#1c1c1c; color:#ffffff; }

        /* Get Started Now button */
        QToolButton#getStartedBtn {
            background:#5a5a5a; color:#dcdcdc;
            border:1px solid #6a6a6a; border-radius:0;
            padding:8px 14px; font-size:12px;
        }
        QToolButton#getStartedBtn:hover  { background:#6a6a6a; }

        /* Manage / + New / Open header buttons */
        QToolButton#actionBtn {
            background:#5a5a5a; color:#dcdcdc;
            border:1px solid #6a6a6a; border-radius:0;
            padding:6px 14px; font-size:12px;
        }
        QToolButton#actionBtn:hover  { background:#6a6a6a; }

        /* Bottom-left link buttons (Account / Community / etc.) */
        QToolButton#footerLink {
            background:transparent; color:#cccccc;
            border:none; padding:6px 0; font-size:12px;
            text-align:left;
        }
        QToolButton#footerLink:hover { color:#ffffff; }

        QScrollArea, QScrollArea > QWidget > QWidget {
            background:transparent; border:none;
        }
    )");

    // Outer = horizontal: [side bar | content area]
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    root->addWidget(buildSideBar(), 0);

    m_contentStack = new QStackedWidget;
    m_contentStack->setObjectName("welcomeContent");
    m_contentStack->addWidget(buildProjectsView());   // 0
    m_contentStack->addWidget(buildExamplesView());   // 1
    m_contentStack->addWidget(buildTutorialsView());  // 2
    root->addWidget(m_contentStack, 1);

    m_tabProjects->setChecked(true);
    onSubTabClicked(TabProjects);
}

// ─── Side bar ─────────────────────────────────────────────────────────────
QWidget *WelcomePage::buildSideBar() {
    auto *side = new QWidget;
    side->setObjectName("welcomeSidebar");
    side->setFixedWidth(250);

    auto *col = new QVBoxLayout(side);
    col->setContentsMargins(0, 18, 0, 18);
    col->setSpacing(2);

    // ── Big sub-tab buttons ─────────────────────────────────
    m_subTabGroup = new QButtonGroup(this);
    m_subTabGroup->setExclusive(true);

    auto makeTab = [&](const QString &text, int id) -> QToolButton* {
        auto *b = new QToolButton;
        b->setObjectName("subTab");
        b->setText("    " + text);
        b->setCheckable(true);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        b->setMinimumHeight(40);
        m_subTabGroup->addButton(b, id);
        col->addWidget(b);
        return b;
    };
    m_tabProjects  = makeTab("Projects",  TabProjects);
    m_tabExamples  = makeTab("Examples",  TabExamples);
    m_tabTutorials = makeTab("Tutorials", TabTutorials);

    col->addSpacing(28);

    // ── "New to Nexor?" promo block ─────────────────────────
    auto *promoTitle = new QLabel("New to Nexor?");
    promoTitle->setObjectName("promoTitle");
    auto *promoBody = new QLabel(
        "Learn how to build atomic activities, "
        "design forms, and run them on the Nexor runtime.");
    promoBody->setObjectName("promoBody");
    promoBody->setWordWrap(true);

    auto *promoCol = new QVBoxLayout;
    promoCol->setContentsMargins(20, 0, 20, 0);
    promoCol->setSpacing(8);
    promoCol->addWidget(promoTitle);
    promoCol->addWidget(promoBody);

    m_getStartedBtn = new QToolButton;
    m_getStartedBtn->setObjectName("getStartedBtn");
    m_getStartedBtn->setText("Get Started Now");
    m_getStartedBtn->setCursor(Qt::PointingHandCursor);
    m_getStartedBtn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    auto *getRow = new QHBoxLayout;
    getRow->setContentsMargins(0, 6, 0, 0);
    getRow->addWidget(m_getStartedBtn);
    getRow->addStretch();
    promoCol->addLayout(getRow);

    col->addLayout(promoCol);

    col->addStretch();

    // ── Bottom links (Account / Community / Blogs / User Guide) ─
    auto *linksCol = new QVBoxLayout;
    linksCol->setContentsMargins(20, 0, 20, 0);
    linksCol->setSpacing(2);

    linksCol->addWidget(makeFooterLink("◯", "Account",          this));
    linksCol->addWidget(makeFooterLink("▭", "Online Community", this));
    linksCol->addWidget(makeFooterLink("≡", "Blogs",            this));
    linksCol->addWidget(makeFooterLink("?", "User Guide",       this));

    col->addLayout(linksCol);

    connect(m_subTabGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &WelcomePage::onSubTabClicked);
    connect(m_getStartedBtn, &QToolButton::clicked,
            this, &WelcomePage::getStartedRequested);

    return side;
}

// ─── Main content view: Projects (Sessions + Projects two columns) ───────
QWidget *WelcomePage::buildProjectsView() {
    auto *page = new QWidget;
    page->setStyleSheet("background:#4d4d4d;");

    auto *root = new QHBoxLayout(page);
    root->setContentsMargins(40, 28, 40, 28);
    root->setSpacing(40);

    // ────────── Sessions column ──────────
    auto *sessionsCol = new QVBoxLayout;
    sessionsCol->setSpacing(12);

    auto *sessionsHeader = new QHBoxLayout;
    auto *sessionsTitle = new QLabel("Sessions");
    sessionsTitle->setObjectName("sectionTitle");
    sessionsHeader->addWidget(sessionsTitle);
    sessionsHeader->addSpacing(14);
    m_manageSessionsBtn = makeActionBtn("⚙", "Manage", page);
    sessionsHeader->addWidget(m_manageSessionsBtn);
    sessionsHeader->addStretch();
    sessionsCol->addLayout(sessionsHeader);

    auto *sessionsRows = new QVBoxLayout;
    sessionsRows->setSpacing(0);
    sessionsRows->setContentsMargins(0, 6, 0, 0);
    rebuildSessionList(sessionsRows);
    sessionsCol->addLayout(sessionsRows);
    sessionsCol->addStretch();

    auto *sessionsWrap = new QWidget;
    sessionsWrap->setLayout(sessionsCol);
    sessionsWrap->setMinimumWidth(280);
    sessionsWrap->setMaximumWidth(320);
    root->addWidget(sessionsWrap, 0);

    // ────────── Projects column ──────────
    auto *projectsCol = new QVBoxLayout;
    projectsCol->setSpacing(12);

    auto *projectsHeader = new QHBoxLayout;
    auto *projectsTitle = new QLabel("Projects");
    projectsTitle->setObjectName("sectionTitle");
    projectsHeader->addWidget(projectsTitle);
    projectsHeader->addSpacing(14);
    m_newBtn  = makeActionBtn("+", "New",  page);
    m_openBtn = makeActionBtn("▥", "Open", page);
    projectsHeader->addWidget(m_newBtn);
    projectsHeader->addWidget(m_openBtn);
    projectsHeader->addStretch();
    projectsCol->addLayout(projectsHeader);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *scrollBody = new QWidget;
    m_recentRowsLayout = new QVBoxLayout(scrollBody);
    m_recentRowsLayout->setContentsMargins(0, 6, 0, 0);
    m_recentRowsLayout->setSpacing(0);
    m_recentRowsLayout->addStretch();   // pushes rows to top
    scroll->setWidget(scrollBody);
    projectsCol->addWidget(scroll, 1);

    rebuildRecentProjects();

    root->addLayout(projectsCol, 1);

    connect(m_newBtn,  &QToolButton::clicked, this, &WelcomePage::newProjectRequested);
    connect(m_openBtn, &QToolButton::clicked, this, &WelcomePage::openProjectRequested);

    return page;
}

QWidget *WelcomePage::buildExamplesView() {
    auto *page = new QWidget;
    page->setStyleSheet("background:#4d4d4d;");
    auto *col = new QVBoxLayout(page);
    col->setContentsMargins(40, 28, 40, 28);
    col->setSpacing(12);

    auto *t = new QLabel("Examples"); t->setObjectName("sectionTitle");
    col->addWidget(t);

    auto *rows = new QVBoxLayout;
    rows->setContentsMargins(0, 6, 0, 0);
    rows->setSpacing(0);
    rebuildExampleList(rows);
    col->addLayout(rows);
    col->addStretch();
    return page;
}

QWidget *WelcomePage::buildTutorialsView() {
    auto *page = new QWidget;
    page->setStyleSheet("background:#4d4d4d;");
    auto *col = new QVBoxLayout(page);
    col->setContentsMargins(40, 28, 40, 28);
    col->setSpacing(12);

    auto *t = new QLabel("Tutorials"); t->setObjectName("sectionTitle");
    col->addWidget(t);

    auto *rows = new QVBoxLayout;
    rows->setContentsMargins(0, 6, 0, 0);
    rows->setSpacing(0);
    rebuildTutorialList(rows);
    col->addLayout(rows);
    col->addStretch();
    return page;
}

// ─── Row builders ─────────────────────────────────────────────────────────
void WelcomePage::rebuildSessionList(QVBoxLayout *into) {
    auto *row = new WelcomeRow(1, WelcomeRow::IconPlay,
                               "default", "(last session)", into->parentWidget());
    into->addWidget(row);
}

void WelcomePage::rebuildRecentProjects() {
    if (!m_recentRowsLayout) return;

    // Clear all but the trailing stretch
    while (m_recentRowsLayout->count() > 1) {
        QLayoutItem *it = m_recentRowsLayout->takeAt(0);
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }

    if (m_recentPaths.isEmpty()) {
        auto *empty = new QLabel(
            "No recent projects.  Click  + New  to create one.");
        empty->setStyleSheet("color:#8a8a8a; font-size:12px; padding:8px 0;");
        m_recentRowsLayout->insertWidget(m_recentRowsLayout->count() - 1, empty);
        return;
    }

    int n = 1;
    for (const QString &path : m_recentPaths) {
        QFileInfo fi(path);
        auto *row = new WelcomeRow(n++, WelcomeRow::IconFolder,
                                   fi.completeBaseName(),
                                   fi.absoluteFilePath());
        connect(row, &QAbstractButton::clicked, this, [this, path]{
            emit recentProjectActivated(path);
        });
        m_recentRowsLayout->insertWidget(m_recentRowsLayout->count() - 1, row);
    }
}

void WelcomePage::rebuildExampleList(QVBoxLayout *into) {
    struct Ex { QString title; QString subtitle; };
    QVector<Ex> items = {
        {"Hello World",        "Single-form atomic activity that prints to console"},
        {"Calculator",         "Two-input form with code-behind doing arithmetic"},
        {"Account Book",       "Multi-form activity with shared global variables"},
        {"Web Service Client", "Demonstrates Resources and HTTP calls"},
    };
    int n = 1;
    for (const Ex &e : items) {
        auto *row = new WelcomeRow(n++, WelcomeRow::IconFolder,
                                   e.title, e.subtitle, into->parentWidget());
        QString name = e.title;
        connect(row, &QAbstractButton::clicked, this, [this, name]{
            emit exampleActivated(name);
        });
        into->addWidget(row);
    }
}

void WelcomePage::rebuildTutorialList(QVBoxLayout *into) {
    struct T { QString title; QString subtitle; };
    QVector<T> items = {
        {"Your First Activity", "Create a project and run an activity"},
        {"Designing Forms",     "Drag-and-drop UI in the form designer"},
        {"The Nexor Language",  "Subs, variables, and global state"},
        {"Connecting to Core",  "Talk to the Nexor backend from Flux"},
    };
    int n = 1;
    for (const T &t : items) {
        auto *row = new WelcomeRow(n++, WelcomeRow::IconFolder,
                                   t.title, t.subtitle, into->parentWidget());
        into->addWidget(row);
    }
}

// ─── Sub-tab switch ───────────────────────────────────────────────────────
void WelcomePage::onSubTabClicked(int tab) {
    if (m_contentStack) m_contentStack->setCurrentIndex(tab);
}

void WelcomePage::setRecentProjects(const QStringList &paths) {
    m_recentPaths = paths;
    rebuildRecentProjects();
}
