#include "WelcomePage.h"

#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileInfo>

WelcomePage::WelcomePage(QWidget *parent) : QWidget(parent) {
    setStyleSheet(R"(
        QWidget#welcomePage { background:#13151b; }
        QLabel#brand        { color:#5b8cff; font-size:42px; font-weight:300;
                              letter-spacing:6px; }
        QLabel#tagline      { color:#6b7280; font-size:14px; letter-spacing:2px; }
        QLabel#sectionLabel { color:#8a95a3; font-size:11px; font-weight:600;
                              letter-spacing:2px; }
        QPushButton#bigAction {
            background:#1f2937; color:#dce1e7; border:1px solid #2a3655;
            border-radius:8px; padding:14px 18px; font-size:13px;
            text-align:left;
        }
        QPushButton#bigAction:hover { background:#243049; border-color:#5b8cff; }
        QListWidget {
            background:#1b1d23; color:#dce1e7; border:1px solid #262932;
            border-radius:6px; font-size:13px; padding:4px;
        }
        QListWidget::item          { padding:8px 10px; }
        QListWidget::item:hover    { background:#243049; }
        QListWidget::item:selected { background:#1e3a5f; color:#ffffff; }
    )");
    setObjectName("welcomePage");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(48, 36, 48, 36);
    root->setSpacing(24);

    // Brand header
    auto *brand   = new QLabel("NEXOR  STUDIO");        brand->setObjectName("brand");
    auto *tagline = new QLabel("Build. Compose. Run."); tagline->setObjectName("tagline");
    root->addWidget(brand);
    root->addWidget(tagline);

    // Body row: actions + recent projects + examples
    auto *body = new QHBoxLayout;
    body->setSpacing(28);

    // ── Actions column ──
    auto *actionsCol = new QVBoxLayout;
    actionsCol->setSpacing(10);
    auto *actionsLbl = new QLabel("ACTIONS"); actionsLbl->setObjectName("sectionLabel");
    actionsCol->addWidget(actionsLbl);

    auto *newBtn  = new QPushButton(" +  New Project");   newBtn->setObjectName("bigAction");
    auto *openBtn = new QPushButton(" ⇪  Open Project");  openBtn->setObjectName("bigAction");
    actionsCol->addWidget(newBtn);
    actionsCol->addWidget(openBtn);
    actionsCol->addStretch();
    body->addLayout(actionsCol, 0);

    // ── Recent projects column ──
    auto *recentCol = new QVBoxLayout;
    recentCol->setSpacing(8);
    auto *recentLbl = new QLabel("RECENT PROJECTS"); recentLbl->setObjectName("sectionLabel");
    recentCol->addWidget(recentLbl);
    m_recentList = new QListWidget;
    m_recentList->setMinimumWidth(280);
    recentCol->addWidget(m_recentList, 1);
    body->addLayout(recentCol, 1);

    // ── Examples column ──
    auto *exCol = new QVBoxLayout;
    exCol->setSpacing(8);
    auto *exLbl = new QLabel("EXAMPLES"); exLbl->setObjectName("sectionLabel");
    exCol->addWidget(exLbl);
    m_exampleList = new QListWidget;
    m_exampleList->addItem("Hello World — single-form atomic activity");
    m_exampleList->addItem("Calculator — two-input form with code-behind");
    m_exampleList->addItem("Account Book — multi-form, shared globals");
    m_exampleList->addItem("Web Service Client — uses Resources");
    exCol->addWidget(m_exampleList, 1);
    body->addLayout(exCol, 1);

    root->addLayout(body, 1);

    connect(newBtn,  &QPushButton::clicked, this, &WelcomePage::newProjectRequested);
    connect(openBtn, &QPushButton::clicked, this, &WelcomePage::openProjectRequested);
    connect(m_recentList,  &QListWidget::itemActivated, this, [this](QListWidgetItem *it){
        if (it) emit recentProjectActivated(it->data(Qt::UserRole).toString());
    });
    connect(m_exampleList, &QListWidget::itemActivated, this, [this](QListWidgetItem *it){
        if (it) emit exampleActivated(it->text());
    });
}

void WelcomePage::setRecentProjects(const QStringList &paths) {
    m_recentList->clear();
    for (const QString &p : paths) {
        QFileInfo fi(p);
        auto *it = new QListWidgetItem(QString("%1\n   %2")
                                         .arg(fi.completeBaseName(), fi.absoluteFilePath()));
        it->setData(Qt::UserRole, fi.absoluteFilePath());
        m_recentList->addItem(it);
    }
    if (paths.isEmpty()) {
        auto *empty = new QListWidgetItem("(no recent projects)");
        empty->setFlags(Qt::NoItemFlags);
        m_recentList->addItem(empty);
    }
}
