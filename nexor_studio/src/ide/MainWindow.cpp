#include "MainWindow.h"
#include "FancyTabBar.h"
#include "CentralStack.h"
#include "OutputPane.h"

#include "welcome/WelcomePage.h"
#include "editor/CodeEditor.h"
#include "editor/EditorView.h"
#include "designer/FormCanvas.h"
#include "designer/DesignerView.h"
#include "designer/PropertyPanel.h"
#include "designer/WidgetFactory.h"
#include "dialogs/TabOrderDialog.h"
#include "runtime/FormRunner.h"
#include "language/NexorRuntime.h"
#include "project/Project.h"

#include <QRegularExpression>
#include <QTextCursor>
#include "project/Activity.h"
#include "project/ProjectTree.h"
#include "dialogs/NewProjectDialog.h"
#include "dialogs/NewActivityDialog.h"
#include "dialogs/NewSheetDialog.h"
#include "dialogs/NewProcessDialog.h"
#include "sheet/SheetEditor.h"
#include "process/ProcessEditor.h"
#include "project/Sheet.h"
#include "project/Process.h"
#include "runtime/ProcessEngine.h"
#include "build/PackageBuilder.h"
#include "build/PackageReader.h"

#include <QInputDialog>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QFileInfo>
#include <QDir>

#include <QMenuBar>
#include <QStatusBar>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabBar(nullptr)
    , m_central(nullptr)
    , m_projectTree(nullptr)
    , m_projectPanel(nullptr)
    , m_outputPane(nullptr)
    , m_horizontalSplit(nullptr)
    , m_verticalSplit(nullptr)
    , m_toggleProjectPanelAction(nullptr)
    , m_toggleOutputPaneAction(nullptr)
    , m_statusModeLabel(nullptr) {

    setWindowTitle("Nexor Studio");
    resize(1320, 860);
    setupUi();
    buildMenus();

    // Initial mode: Welcome
    applyModeLayout(FancyTabBar::ModeWelcome);
    appendOutput("Nexor Studio started.", "#5b8cff");

    m_central->welcomePage()->setRecentProjects(loadRecentProjects());
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setStyleSheet(R"(
        QMainWindow      { background:#13151b; }
        QMenuBar         { background:#0d0e12; color:#dce1e7;
                           border-bottom:1px solid #1e2030; }
        QMenuBar::item   { padding:5px 12px; }
        QMenuBar::item:selected { background:#1e3a5f; }
        QMenu            { background:#1b1d23; color:#dce1e7;
                           border:1px solid #2a3655; }
        QMenu::item:selected { background:#1e3a5f; }
        QStatusBar       { background:#0d0e12; color:#8a95a3;
                           border-top:1px solid #1e2030; padding:0 8px; }
        QStatusBar::item { border:none; }
    )");

    // Root layout: [ FancyTabBar | verticalSplitter ]
    auto *shell = new QWidget(this);
    auto *row   = new QHBoxLayout(shell);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    m_tabBar = new FancyTabBar(shell);
    row->addWidget(m_tabBar);

    // Vertical splitter: top is the [ProjectPanel | Central], bottom is OutputPane
    m_verticalSplit = new QSplitter(Qt::Vertical, shell);
    m_verticalSplit->setHandleWidth(1);
    m_verticalSplit->setChildrenCollapsible(false);
    m_verticalSplit->setStyleSheet("QSplitter::handle{ background:#1e2030; }");

    // Horizontal splitter: project panel | central
    m_horizontalSplit = new QSplitter(Qt::Horizontal, m_verticalSplit);
    m_horizontalSplit->setHandleWidth(1);
    m_horizontalSplit->setChildrenCollapsible(false);
    m_horizontalSplit->setStyleSheet("QSplitter::handle{ background:#1e2030; }");

    // Project panel (header + tree)
    m_projectPanel = new QWidget(m_horizontalSplit);
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

    // Central (welcome / editor / designer / build / debug)
    m_central = new CentralStack(m_horizontalSplit);

    m_horizontalSplit->addWidget(m_projectPanel);
    m_horizontalSplit->addWidget(m_central);
    m_horizontalSplit->setStretchFactor(0, 0);
    m_horizontalSplit->setStretchFactor(1, 1);
    m_horizontalSplit->setSizes({ 280, 1000 });

    // Output pane at bottom
    m_outputPane = new OutputPane(m_verticalSplit);

    m_verticalSplit->addWidget(m_horizontalSplit);
    m_verticalSplit->addWidget(m_outputPane);
    m_verticalSplit->setStretchFactor(0, 1);
    m_verticalSplit->setStretchFactor(1, 0);
    m_verticalSplit->setSizes({ 600, 200 });

    row->addWidget(m_verticalSplit, 1);
    setCentralWidget(shell);

    // Status bar
    m_statusModeLabel = new QLabel("Welcome");
    m_statusModeLabel->setStyleSheet("color:#5b8cff; font-weight:600;");
    statusBar()->addPermanentWidget(m_statusModeLabel);
    statusBar()->showMessage("Ready");

    // ── Wiring ────────────────────────────────────────────────────────
    connect(m_tabBar, &FancyTabBar::currentChanged,
            this, &MainWindow::onModeChanged);

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
    connect(m_projectTree, &ProjectTree::sheetActivated,
            this, &MainWindow::onSheetActivated);
    connect(m_projectTree, &ProjectTree::processActivated,
            this, &MainWindow::onProcessActivated);

    // ProcessEditor's "▶ Run Process" button.
    connect(m_central->processEditor(), &ProcessEditor::runRequested,
            this, &MainWindow::onRunProcessRequested);

    // PropertyPanel "+ generate handler" button → insert stub into the form
    // code (in the editor if it's already showing this form's code; in the
    // canvas otherwise) and switch to Edit mode focused on the new sub.
    auto generateHandler = [this](const QString &target, const QString &event){
        QString subName = target + "_" + event;
        auto *editor = m_central->codeEditor();
        auto *canvas = m_central->formCanvas();
        if (!canvas || canvas->currentFormPath().isEmpty()) return;

        // Make sure the editor is showing this form's code.
        if (editor->currentKind() != CodeEditor::KindForm
         || editor->currentFilePath() != canvas->currentFormPath()) {
            editor->loadForm(canvas->currentFormPath());
        }

        // Find an existing handler with that name.
        QRegularExpression re(
            QString(R"(\bSub\s+%1\s*\()").arg(QRegularExpression::escape(subName)),
            QRegularExpression::CaseInsensitiveOption);
        auto match = re.match(editor->toPlainText());

        if (match.hasMatch()) {
            // Just navigate to it.
            QTextCursor c(editor->document());
            c.setPosition(match.capturedStart());
            editor->setTextCursor(c);
        } else {
            // Append a stub.
            QString stub = QString(
                "\nSub %1()\n"
                "    ' TODO: handle %2\n"
                "End Sub\n"
            ).arg(subName, event);
            QTextCursor c = editor->textCursor();
            c.movePosition(QTextCursor::End);
            c.insertText(stub);
            // Move cursor inside the new sub body.
            c.movePosition(QTextCursor::Up,   QTextCursor::MoveAnchor, 2);
            c.movePosition(QTextCursor::EndOfLine);
            editor->setTextCursor(c);
            appendOutput("Inserted handler: " + subName, "#a3e635");
        }
        editor->ensureCursorVisible();
        editor->setFocus();

        // Switch to Edit mode.
        m_tabBar->setCurrentMode(FancyTabBar::ModeEdit);
        m_central->showPage(CentralStack::PageEditor);
    };

    connect(m_central->designerView()->propertyPanel(),
            &PropertyPanel::eventHandlerRequested,
            this, generateHandler);

    // EditorView's Object/Procedure dropdown — same code path.
    connect(m_central->editorView(),
            &EditorView::eventHandlerRequested,
            this, generateHandler);

    // F2: double-click a widget on the canvas → default event for its type.
    connect(m_central->formCanvas(), &FormCanvas::widgetDoubleClicked,
            this, [generateHandler](const QString &name, const QString &type){
                generateHandler(name, WidgetFactory::defaultEvent(type));
            });

    // F2: double-click empty form area → Form_Load.
    connect(m_central->formCanvas(), &FormCanvas::formDoubleClicked,
            this, [this, generateHandler]{
                if (!m_central->formCanvas()
                 || m_central->formCanvas()->currentFormPath().isEmpty()) return;
                QString formId = QFileInfo(
                    m_central->formCanvas()->currentFormPath()).completeBaseName();
                generateHandler(formId, "Load");
            });
}

void MainWindow::buildMenus() {
    auto *fileMenu  = menuBar()->addMenu("&File");
    fileMenu->addAction("&New Project...",  this, &MainWindow::onNewProject,  QKeySequence::New);
    fileMenu->addAction("&Open Project...", this, &MainWindow::onOpenProject, QKeySequence::Open);
    fileMenu->addAction("&Close Project",   this, &MainWindow::onCloseProject);
    fileMenu->addSeparator();
    // Save: coordinates editor and canvas so we never lose either side's edits.
    fileMenu->addAction("&Save", this, [this]{
        auto *ed = m_central->codeEditor();
        auto *cv = m_central->formCanvas();
        bool any = false;

        // Same .frm in both editor (form mode) and canvas → single combined save.
        bool sameFrm = (ed && cv
                     && ed->currentKind() == CodeEditor::KindForm
                     && !ed->currentFilePath().isEmpty()
                     && ed->currentFilePath() == cv->currentFormPath());

        if (sameFrm) {
            cv->setCode(ed->toPlainText());     // push editor text into canvas
            if (cv->saveForm()) {
                appendOutput("Saved form: " + cv->currentFormPath(), "#22c55e");
                any = true;
            }
        } else {
            // Independent saves of whichever side has content.
            if (ed) {
                if (ed->currentKind() == CodeEditor::KindActivity && ed->saveActivity()) {
                    appendOutput("Saved activity: " + ed->currentFilePath(), "#22c55e");
                    any = true;
                } else if (ed->currentKind() == CodeEditor::KindForm && ed->saveForm()) {
                    appendOutput("Saved form code: " + ed->currentFilePath(), "#22c55e");
                    any = true;
                }
            }
            if (cv && !cv->currentFormPath().isEmpty()) {
                if (cv->saveForm()) {
                    appendOutput("Saved form: " + cv->currentFormPath(), "#22c55e");
                    any = true;
                }
            }
        }

        if (!any) statusBar()->showMessage("Nothing to save.", 2000);
    }, QKeySequence::Save);
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

    m_toggleOutputPaneAction = new QAction("Output Pane", this);
    m_toggleOutputPaneAction->setCheckable(true);
    m_toggleOutputPaneAction->setChecked(true);
    m_toggleOutputPaneAction->setShortcut(QKeySequence("Alt+9"));
    connect(m_toggleOutputPaneAction, &QAction::toggled, this, [this](bool on){
        if (m_outputPane) m_outputPane->setVisible(on);
    });
    viewMenu->addAction(m_toggleOutputPaneAction);

    viewMenu->addSeparator();
    viewMenu->addAction("&Tab Order...", this, [this]{
        if (!m_central->formCanvas()
         || m_central->formCanvas()->currentFormPath().isEmpty()) {
            QMessageBox::information(this, "Tab Order",
                "Open a form first.");
            return;
        }
        TabOrderDialog dlg(m_central->formCanvas(), this);
        dlg.exec();
    });

    auto *buildMenu = menuBar()->addMenu("&Build");
    buildMenu->addAction("&Build Package...", this, [this]{
        if (!m_project) {
            QMessageBox::information(this, "Build Package",
                "Open or create a project first.");
            return;
        }
        // Persist any unsaved editor / canvas changes so the package picks
        // up exactly what the developer sees on screen.
        if (auto *ed = m_central->codeEditor()) {
            if (ed->currentKind() == CodeEditor::KindActivity) ed->saveActivity();
            else if (ed->currentKind() == CodeEditor::KindForm) ed->saveForm();
        }
        if (auto *cv = m_central->formCanvas())
            if (!cv->currentFormPath().isEmpty()) cv->saveForm();

        // Last-built version sticks per-project so the user only types it
        // once for v0.1.0 → v0.1.1 → … bumps.
        QSettings s;
        QString key  = "Build/LastVersion/" + m_project->meta().id;
        QString prev = s.value(key, "0.1.0").toString();
        bool ok = false;
        QString version = QInputDialog::getText(this, "Build Package",
            "Version (SemVer):", QLineEdit::Normal, prev, &ok).trimmed();
        if (!ok || version.isEmpty()) return;
        s.setValue(key, version);

        appendOutput(QString("Building %1 v%2…")
                        .arg(m_project->meta().id, version), "#5b8cff");

        auto res = nx::PackageBuilder::buildAndWrite(*m_project, version);
        if (!res.ok) {
            appendOutput("Build failed: " + res.error, "#ef4444");
            QMessageBox::warning(this, "Build Package", res.error);
            return;
        }

        // Round-trip verification — read the package back and confirm the
        // hash holds.  Catches anything stray that landed between writer and
        // reader without the developer having to ship a broken artifact.
        auto rd = nx::PackageReader::fromFile(res.outputPath);
        if (rd.status == nx::PackageReader::Status::HashMismatch) {
            appendOutput("WARN: hash mismatch on round-trip — " + rd.message, "#facc15");
        } else if (rd.status != nx::PackageReader::Status::Ok) {
            appendOutput("WARN: package wrote but reader rejected it — " + rd.message,
                         "#facc15");
        }

        appendOutput(QString("Wrote %1 (%2 activities, %3 forms, %4 sheets, %5 processes)")
                        .arg(res.outputPath)
                        .arg(rd.package.activities.size())
                        .arg(rd.package.forms.size())
                        .arg(rd.package.sheets.size())
                        .arg(rd.package.processes.size()),
                     "#22c55e");
        appendOutput("Hash: " + res.meta.hash.left(16) + "…", "#8a95a3");
        statusBar()->showMessage("Package built — " + res.outputPath, 5000);
    }, QKeySequence("Ctrl+B"));

    buildMenu->addAction("&Open Output Folder", this, [this]{
        if (!m_project) return;
        QString dist = m_project->rootDir() + "/dist";
        if (!QDir(dist).exists()) {
            statusBar()->showMessage("No build output yet — run Build Package first.", 3000);
            return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(dist));
    });
    buildMenu->addSeparator();

    // Publish — uploads the most recently built .nexor for this project to
    // a Nexor Core instance.  The URL is per-machine settings so the same
    // Studio install can target dev / staging / prod by editing it.
    buildMenu->addAction("&Publish to Core...", this, [this]{
        if (!m_project) {
            QMessageBox::information(this, "Publish",
                "Open or create a project first.");
            return;
        }
        QSettings s;
        QString verKey = "Build/LastVersion/" + m_project->meta().id;
        QString version = s.value(verKey).toString();
        if (version.isEmpty()) {
            QMessageBox::information(this, "Publish",
                "Build the project at least once before publishing.");
            return;
        }
        QString fileName = QString("%1-%2.nexor").arg(m_project->meta().id, version);
        QString filePath = m_project->rootDir() + "/dist/" + fileName;
        if (!QFileInfo::exists(filePath)) {
            QMessageBox::warning(this, "Publish",
                "Package not found:\n" + filePath +
                "\n\nRun Build Package first.");
            return;
        }

        QString defaultUrl = s.value("Publish/CoreUrl", "http://localhost:7421").toString();
        bool ok = false;
        QString coreUrl = QInputDialog::getText(this, "Publish to Core",
            "Nexor Core URL:", QLineEdit::Normal, defaultUrl, &ok).trimmed();
        if (!ok || coreUrl.isEmpty()) return;
        s.setValue("Publish/CoreUrl", coreUrl);

        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly)) {
            appendOutput("Publish failed: cannot read " + filePath, "#ef4444");
            return;
        }
        QByteArray bytes = f.readAll();
        f.close();

        appendOutput(QString("Publishing %1 (%2 bytes) → %3 …")
                        .arg(fileName).arg(bytes.size()).arg(coreUrl),
                     "#5b8cff");

        auto *nam = new QNetworkAccessManager(this);
        QUrl url(coreUrl + "/api/v1/packages");
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-nexor-package");
        req.setRawHeader("Accept", "application/json");
        QNetworkReply *reply = nam->post(req, bytes);
        connect(reply, &QNetworkReply::finished, this, [this, reply, nam, fileName]{
            QByteArray body = reply->readAll();
            int status = reply->attribute(
                QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (reply->error() != QNetworkReply::NoError) {
                appendOutput(QString("Publish error (%1): %2").arg(status)
                                .arg(reply->errorString()), "#ef4444");
                if (!body.isEmpty())
                    appendOutput("  body: " + QString::fromUtf8(body), "#ef4444");
            } else {
                appendOutput(QString("Published %1 → HTTP %2")
                                .arg(fileName).arg(status), "#22c55e");
                appendOutput("  " + QString::fromUtf8(body), "#8a95a3");
            }
            reply->deleteLater();
            nam->deleteLater();
        });
    }, QKeySequence("Ctrl+Shift+P"));

    buildMenu->addAction("Clean Project")->setEnabled(false);

    auto *runMenu = menuBar()->addMenu("&Run");
    runMenu->addAction("&Run Form", this, [this]{
        if (!m_central->formCanvas()
         || m_central->formCanvas()->currentFormPath().isEmpty()) {
            QMessageBox::information(this, "Run Form",
                "Open a form first (double-click a .frm in the project tree).");
            return;
        }
        // Save first so the runner picks up the latest design + code
        if (m_central->codeEditor()
            && m_central->codeEditor()->currentKind() == CodeEditor::KindForm
            && m_central->codeEditor()->currentFilePath() ==
                                         m_central->formCanvas()->currentFormPath()) {
            m_central->formCanvas()->setCode(m_central->codeEditor()->toPlainText());
        }
        m_central->formCanvas()->saveForm();
        QString path = m_central->formCanvas()->currentFormPath();
        appendOutput("Run form: " + path, "#5b8cff");
        // Pipe Print + runtime errors back to the OUTPUT pane.
        FormRunner::runForm(path, this,
            [this](const QString &line) { appendOutput(line, "#dce1e7"); },
            [this](const QString &err)  { appendOutput("ERROR: " + err, "#ef4444"); },
            m_project.get());
    }, QKeySequence(Qt::Key_F5));

    runMenu->addAction("Run &Activity (Sub Main)", this, [this]{
        // Runs the current activity's Sub Main from the editor's text.
        if (!m_central->codeEditor()
         || m_central->codeEditor()->currentKind() != CodeEditor::KindActivity) {
            QMessageBox::information(this, "Run Activity",
                "Open an activity (.aba) in the editor first.");
            return;
        }
        // Persist what's in the editor before running.
        m_central->codeEditor()->saveActivity();
        QString unit = QFileInfo(m_central->codeEditor()->currentFilePath()).fileName();
        QString src  = m_central->codeEditor()->toPlainText();
        appendOutput("Run activity: " + unit, "#5b8cff");

        nx::NexorRuntime rt;
        rt.setOutput([this](const QString &line) { appendOutput(line, "#dce1e7"); });
        rt.setError ([this](const QString &err)  { appendOutput("ERROR: " + err, "#ef4444"); });
        rt.registerProjectSheets(m_project.get());
        if (!rt.compile(src, unit)) {
            appendOutput("Compile error: " + rt.lastError(), "#ef4444");
            return;
        }
        if (!rt.hasSub("Main")) {
            appendOutput("No Sub Main() defined in this activity.", "#facc15");
            return;
        }
        rt.call("Main");
        appendOutput("Activity finished.", "#22c55e");
    }, QKeySequence("Ctrl+F5"));

    auto *dbgMenu = menuBar()->addMenu("&Debug");
    dbgMenu->addAction("Start Debugging")->setEnabled(false);
    dbgMenu->addAction("Toggle Breakpoint")->setEnabled(false);

    auto *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About Nexor Studio", this, &MainWindow::onAbout);
}

// ─── Mode-driven layout ───────────────────────────────────────────────────

void MainWindow::onModeChanged(int mode) {
    applyModeLayout(mode);
    auto page = static_cast<CentralStack::Page>(
        mode == FancyTabBar::ModeWelcome ? CentralStack::PageWelcome :
        mode == FancyTabBar::ModeEdit    ? CentralStack::PageEditor  :
        mode == FancyTabBar::ModeDesign  ? CentralStack::PageDesigner:
        mode == FancyTabBar::ModeDebug   ? CentralStack::PageDebug   :
                                           CentralStack::PageBuild   );
    m_central->showPage(page);
}

void MainWindow::applyModeLayout(int mode) {
    static const char *names[] = {
        "Welcome", "Edit", "Design", "Debug", "Projects", "Help"
    };
    if (mode >= 0 && mode < int(sizeof(names)/sizeof(names[0]))) {
        m_statusModeLabel->setText(names[mode]);
    }

    // Project panel:
    //   ALLOWED in Edit / Debug only.
    //   In other modes it is force-hidden AND the View-menu toggle is
    //   disabled so the user cannot bring it back.
    bool allowed = (mode == FancyTabBar::ModeEdit
                 || mode == FancyTabBar::ModeDebug);
    if (m_projectPanel) m_projectPanel->setVisible(allowed);
    if (m_toggleProjectPanelAction) {
        m_toggleProjectPanelAction->blockSignals(true);
        m_toggleProjectPanelAction->setChecked(allowed);
        m_toggleProjectPanelAction->blockSignals(false);
        m_toggleProjectPanelAction->setEnabled(allowed);
    }
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
    m_tabBar->setCurrentMode(FancyTabBar::ModeEdit);  // also triggers layout
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
    m_tabBar->setCurrentMode(FancyTabBar::ModeEdit);
    rememberRecent(m_project->filePath());
    appendOutput("Opened " + m_project->filePath(), "#5b8cff");
    setWindowTitle("Nexor Studio — " + m_project->meta().title);
}

void MainWindow::onCloseProject() {
    if (!m_project) return;
    appendOutput("Closed " + m_project->filePath(), "#8a95a3");
    m_project.reset();
    m_projectTree->setProject(nullptr);
    m_tabBar->setCurrentMode(FancyTabBar::ModeWelcome);
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
    if (!m_project) {
        QMessageBox::information(this, "New Process",
            "Open or create a project first.");
        return;
    }
    NewProcessDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    QString err;
    auto prc = m_project->createProcess(dlg.meta(), &err);
    if (!prc) {
        QMessageBox::warning(this, "New Process", err);
        return;
    }
    m_projectTree->refresh();
    appendOutput(QString("Created process '%1' at %2")
                    .arg(prc->meta().title, prc->filePath()), "#22c55e");
    onProcessActivated(prc->filePath());
}

void MainWindow::onProcessActivated(const QString &absPath) {
    if (!m_central->processEditor()->loadProcess(absPath)) {
        appendOutput("Failed to read process: " + absPath, "#ef4444");
        return;
    }
    appendOutput("Open process: " + absPath, "#c084fc");
    m_central->showPage(CentralStack::PageProcess);
    m_tabBar->setCurrentMode(FancyTabBar::ModeEdit);
}

void MainWindow::onRunProcessRequested(const QString &absPath) {
    appendOutput("Run process: " + absPath, "#5b8cff");
    bool ok = ProcessEngine::runProcess(absPath,
        [this](const QString &line) { appendOutput(line, "#dce1e7"); },
        [this](const QString &er)   { appendOutput("ERROR: " + er, "#ef4444"); },
        m_project.get());
    appendOutput(ok ? "Process finished." : "Process aborted.",
                 ok ? "#22c55e" : "#ef4444");
}

void MainWindow::onNewSheet() {
    if (!m_project) {
        QMessageBox::information(this, "New Sheet",
            "Open or create a project first.");
        return;
    }
    NewSheetDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    QString err;
    auto sht = m_project->createSheet(dlg.meta(), &err);
    if (!sht) {
        QMessageBox::warning(this, "New Sheet", err);
        return;
    }
    m_projectTree->refresh();
    appendOutput(QString("Created sheet '%1' at %2")
                    .arg(sht->meta().title, sht->filePath()), "#22c55e");
    onSheetActivated(sht->filePath());
}

void MainWindow::onSheetActivated(const QString &absPath) {
    if (!m_central->sheetEditor()->loadSheet(absPath)) {
        appendOutput("Failed to read sheet: " + absPath, "#ef4444");
        return;
    }
    appendOutput("Open sheet: " + absPath, "#fbbf24");
    m_central->showPage(CentralStack::PageSheet);
    m_tabBar->setCurrentMode(FancyTabBar::ModeEdit);
}

void MainWindow::onFormActivated(const QString &absPath) {
    // 1. Load the form into the canvas (UI + widgets + code).
    if (!m_central->formCanvas()->loadForm(absPath)) {
        appendOutput("Failed to read form: " + absPath, "#ef4444");
        return;
    }
    appendOutput("Open form: " + absPath, "#a3e635");

    // 2. Load the FORM's own event-driven code into the editor.
    //    (The activity's Sub Main lives in the .aba and is shown when the
    //    user opens the activity itself.)
    if (m_central->codeEditor()->loadForm(absPath))
        appendOutput("Loaded form code: " + absPath, "#5b8cff");

    // 3. Refresh the Object/Procedure dropdowns to list the form + widgets.
    m_central->editorView()->refresh();

    // 4. Switch to Design mode (user can hit EDIT to see the form's code).
    m_tabBar->setCurrentMode(FancyTabBar::ModeDesign);
    m_central->showPage(CentralStack::PageDesigner);
}

void MainWindow::onActivityActivated(const QString &absPath) {
    if (!m_central->codeEditor()->loadActivity(absPath)) {
        appendOutput("Failed to read activity: " + absPath, "#ef4444");
        return;
    }
    appendOutput("Open activity: " + absPath, "#a3e635");
    m_central->editorView()->refresh();   // dropdowns become "(General)" only
    m_tabBar->setCurrentMode(FancyTabBar::ModeEdit);
    m_central->showPage(CentralStack::PageEditor);
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About Nexor Studio",
        "<h2>Nexor Studio</h2>"
        "<p>Version 0.1.0</p>"
        "<p>The Nexor IDE — author atomic activities, design forms, run them.</p>");
}

// ─── Output panel + recent-projects persistence ───────────────────────────

void MainWindow::appendOutput(const QString &line, const QString &color) {
    if (m_outputPane)
        m_outputPane->appendTo(OutputPane::PaneAppOutput, line, color);
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
