#include "MainWindow.h"
#include "SideBar.h"
#include "CentralStack.h"

#include "welcome/WelcomePage.h"
#include "project/Project.h"
#include "project/Activity.h"
#include "project/ProjectTree.h"
#include "dialogs/NewProjectDialog.h"
#include "dialogs/NewActivityDialog.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_sideBar(nullptr)
    , m_central(nullptr)
    , m_projectTree(nullptr)
    , m_projectPanel(nullptr)
    , m_toggleProjectPanelAction(nullptr)
    , m_outputDock(nullptr)
    , m_output(nullptr) {

    setWindowTitle("Nexor Studio");
    resize(1280, 820);
    setupUi();
    buildMenus();
    statusBar()->showMessage("Ready");

    appendOutput("Nexor Studio started.", "#5b8cff");

    // Populate Welcome page recents.
    m_central->welcomePage()->setRecentProjects(loadRecentProjects());
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setStyleSheet(R"(
        QMainWindow         { background:#13151b; }
        QMenuBar            { background:#0d0e12; color:#dce1e7;
                              border-bottom:1px solid #1e2030; }
        QMenuBar::item      { padding:5px 12px; }
        QMenuBar::item:selected { background:#1e3a5f; }
        QMenu               { background:#1b1d23; color:#dce1e7;
                              border:1px solid #2a3655; }
        QMenu::item:selected{ background:#1e3a5f; }
        QStatusBar          { background:#0d0e12; color:#8a95a3;
                              border-top:1px solid #1e2030; }
        QDockWidget         { color:#8a95a3; font-size:11px; font-weight:600;
                              letter-spacing:1px; }
        QDockWidget::title  { background:#13151b; padding:6px 10px;
                              border-bottom:1px solid #1e2030; }
        QPlainTextEdit      { background:#0e1015; color:#dce1e7;
                              font-family:"Consolas","Courier New",monospace;
                              font-size:12px; border:none; }
    )");

    // Layout (Qt Creator-style):
    //   [ SideBar | [ ProjectPanel │ CentralStack ] ]   ← horizontal QSplitter
    //   The bottom-area Output remains a QDockWidget so it can be hidden/popped.

    auto *shell = new QWidget(this);
    auto *row   = new QHBoxLayout(shell);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    m_sideBar = new SideBar(shell);

    auto *splitter = new QSplitter(Qt::Horizontal, shell);
    splitter->setHandleWidth(1);
    splitter->setChildrenCollapsible(false);
    splitter->setStyleSheet("QSplitter::handle{ background:#1e2030; }");

    // ── Project panel (header + tree) ───────────────────────────────────
    m_projectPanel = new QWidget(splitter);
    m_projectPanel->setStyleSheet("background:#13151b;");
    auto *panelCol = new QVBoxLayout(m_projectPanel);
    panelCol->setContentsMargins(0, 0, 0, 0);
    panelCol->setSpacing(0);

    auto *panelHeader = new QLabel("PROJECT", m_projectPanel);
    panelHeader->setStyleSheet(
        "QLabel { background:#0d0e12; color:#8a95a3;"
        " padding:8px 12px; border-bottom:1px solid #1e2030;"
        " font-size:11px; font-weight:600; letter-spacing:2px; }");
    panelCol->addWidget(panelHeader);

    m_projectTree = new ProjectTree(m_projectPanel);
    panelCol->addWidget(m_projectTree, 1);

    m_central = new CentralStack(splitter);

    splitter->addWidget(m_projectPanel);
    splitter->addWidget(m_central);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({280, 1000});

    row->addWidget(m_sideBar);
    row->addWidget(splitter, 1);
    setCentralWidget(shell);

    // Output dock (bottom)
    m_output = new QPlainTextEdit;
    m_output->setReadOnly(true);
    m_outputDock = new QDockWidget("OUTPUT", this);
    m_outputDock->setWidget(m_output);
    m_outputDock->setMinimumHeight(140);
    addDockWidget(Qt::BottomDockWidgetArea, m_outputDock);

    connect(m_sideBar, &SideBar::modeChanged, this, &MainWindow::onModeChanged);

    connect(m_central->welcomePage(), &WelcomePage::newProjectRequested,
            this, &MainWindow::onNewProject);
    connect(m_central->welcomePage(), &WelcomePage::openProjectRequested,
            this, &MainWindow::onOpenProject);
    connect(m_central->welcomePage(), &WelcomePage::recentProjectActivated,
            this, &MainWindow::loadProjectFromFile);

    connect(m_projectTree, &ProjectTree::requestNewAtomicActivity,
            this, &MainWindow::onNewAtomicActivity);
    connect(m_projectTree, &ProjectTree::requestNewProcessActivity,
            this, &MainWindow::onNewProcessActivity);
    connect(m_projectTree, &ProjectTree::requestNewSheet,
            this, &MainWindow::onNewSheet);
    connect(m_projectTree, &ProjectTree::formActivated,
            this, &MainWindow::onFormActivated);
    connect(m_projectTree, &ProjectTree::activityActivated,
            this, &MainWindow::onActivityActivated);
}

void MainWindow::buildMenus() {
    auto *fileMenu  = menuBar()->addMenu("&File");
    fileMenu->addAction("&New Project...",  this, &MainWindow::onNewProject,  QKeySequence::New);
    fileMenu->addAction("&Open Project...", this, &MainWindow::onOpenProject, QKeySequence::Open);
    fileMenu->addAction("&Close Project",   this, &MainWindow::onCloseProject);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence("Ctrl+Q"));

    auto *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("Undo")->setEnabled(false);
    editMenu->addAction("Redo")->setEnabled(false);

    auto *viewMenu = menuBar()->addMenu("&View");
    m_toggleProjectPanelAction = new QAction("Project Panel", this);
    m_toggleProjectPanelAction->setCheckable(true);
    m_toggleProjectPanelAction->setChecked(true);
    m_toggleProjectPanelAction->setShortcut(QKeySequence("Alt+0"));
    connect(m_toggleProjectPanelAction, &QAction::toggled, this, [this](bool on){
        if (m_projectPanel) m_projectPanel->setVisible(on);
    });
    viewMenu->addAction(m_toggleProjectPanelAction);
    viewMenu->addAction(m_outputDock->toggleViewAction());

    auto *buildMenu = menuBar()->addMenu("&Build");
    buildMenu->addAction("Build Project")->setEnabled(false);
    buildMenu->addAction("Clean Project")->setEnabled(false);

    auto *runMenu = menuBar()->addMenu("&Run");
    runMenu->addAction("Run")->setEnabled(false);

    auto *dbgMenu = menuBar()->addMenu("&Debug");
    dbgMenu->addAction("Start Debugging")->setEnabled(false);
    dbgMenu->addAction("Toggle Breakpoint")->setEnabled(false);

    auto *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About Nexor Studio", this, &MainWindow::onAbout);
}

void MainWindow::onModeChanged(int mode) {
    auto page = static_cast<CentralStack::Page>(mode);
    m_central->showPage(page);
}

// ─── Project lifecycle ────────────────────────────────────────────────────

void MainWindow::onNewProject() {
    NewProjectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    ProjectMeta m = dlg.meta();
    QString parent = dlg.parentDirectory();
    QString rootDir = QString("%1/%2").arg(parent, m.id);

    QString err;
    auto p = Project::createOnDisk(m, rootDir, &err);
    if (!p) {
        QMessageBox::warning(this, "New Project", "Could not create project:\n" + err);
        return;
    }

    m_project = std::move(p);
    m_projectTree->setProject(m_project.get());
    m_central->showPage(CentralStack::PageEditor);
    m_sideBar->setMode(SideBar::ModeEdit);
    rememberRecent(m_project->filePath());
    appendOutput(QString("Created project '%1' at %2")
                    .arg(m.title, m_project->filePath()), "#22c55e");
    statusBar()->showMessage("Project created.");
    setWindowTitle("Nexor Studio — " + m_project->meta().title);
}

void MainWindow::onOpenProject() {
    QString path = QFileDialog::getOpenFileName(this, "Open Project",
                                                QString(), "Nexor Project (*.pro)");
    if (path.isEmpty()) return;
    loadProjectFromFile(path);
}

void MainWindow::loadProjectFromFile(const QString &path) {
    auto p = std::make_unique<Project>();
    p->setFilePath(path);
    if (!p->load()) {
        QMessageBox::warning(this, "Open Project", "Failed to read project:\n" + path);
        return;
    }
    m_project = std::move(p);
    m_projectTree->setProject(m_project.get());
    m_central->showPage(CentralStack::PageEditor);
    m_sideBar->setMode(SideBar::ModeEdit);
    rememberRecent(m_project->filePath());
    appendOutput("Opened " + m_project->filePath(), "#5b8cff");
    setWindowTitle("Nexor Studio — " + m_project->meta().title);
}

void MainWindow::onCloseProject() {
    if (!m_project) return;
    appendOutput("Closed " + m_project->filePath(), "#8a95a3");
    m_project.reset();
    m_projectTree->setProject(nullptr);
    m_central->showPage(CentralStack::PageWelcome);
    m_sideBar->setMode(SideBar::ModeProjects);
    setWindowTitle("Nexor Studio");
}

// ─── Tree-driven creation ─────────────────────────────────────────────────

void MainWindow::onNewAtomicActivity() {
    if (!m_project) {
        QMessageBox::information(this, "New Activity",
            "Open or create a project first.");
        return;
    }
    NewActivityDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    QString err;
    auto act = m_project->createAtomicActivity(dlg.meta(), &err);
    if (!act) {
        QMessageBox::warning(this, "New Activity", err);
        return;
    }
    m_projectTree->refresh();
    appendOutput(QString("Created atomic activity '%1' at %2")
                    .arg(act->meta().title, act->filePath()), "#22c55e");
}

void MainWindow::onNewProcessActivity() {
    QMessageBox::information(this, "New Process Activity",
        "Process activities will land in feature/process-activities.");
}

void MainWindow::onNewSheet() {
    QMessageBox::information(this, "New Sheet",
        "Sheets will land in feature/sheets.");
}

void MainWindow::onFormActivated(const QString &absPath) {
    appendOutput("Open form: " + absPath, "#a3e635");
    m_central->showPage(CentralStack::PageDesigner);
    m_sideBar->setMode(SideBar::ModeDesigner);
}

void MainWindow::onActivityActivated(const QString &absPath) {
    appendOutput("Open activity: " + absPath, "#a3e635");
    m_central->showPage(CentralStack::PageEditor);
    m_sideBar->setMode(SideBar::ModeEdit);
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About Nexor Studio",
        "<h2>Nexor Studio</h2>"
        "<p>Version 0.1.0</p>"
        "<p>The Nexor IDE — author atomic activities, design forms, run them.</p>");
}

// ─── Output panel + recent-projects persistence ───────────────────────────

void MainWindow::appendOutput(const QString &line, const QString &color) {
    QString stamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString c = color.isEmpty() ? "#dce1e7" : color;
    m_output->appendHtml(QString("<span style='color:#6b7280'>[%1]</span> "
                                 "<span style='color:%2'>%3</span>")
                          .arg(stamp, c, line.toHtmlEscaped()));
}

void MainWindow::rememberRecent(const QString &path) {
    QSettings s;
    QStringList recents = s.value("Recents/Projects").toStringList();
    recents.removeAll(path);
    recents.prepend(path);
    while (recents.size() > 10) recents.removeLast();
    s.setValue("Recents/Projects", recents);
    m_central->welcomePage()->setRecentProjects(recents);
}

QStringList MainWindow::loadRecentProjects() const {
    QSettings s;
    return s.value("Recents/Projects").toStringList();
}
