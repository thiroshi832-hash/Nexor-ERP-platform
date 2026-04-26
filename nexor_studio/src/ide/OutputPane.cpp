#include "OutputPane.h"

#include <QStackedWidget>
#include <QPlainTextEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QShortcut>
#include <QDateTime>

namespace {
QPlainTextEdit *makePane(QWidget *parent) {
    auto *te = new QPlainTextEdit(parent);
    te->setReadOnly(true);
    te->setFrameShape(QFrame::NoFrame);
    te->setStyleSheet(R"(
        QPlainTextEdit {
            background:#0e1015;
            color:#dce1e7;
            font-family:"Consolas","Courier New",monospace;
            font-size:12px;
            border:none;
            padding:6px 10px;
        }
    )");
    return te;
}
}

OutputPane::OutputPane(QWidget *parent) : QWidget(parent) {
    setObjectName("outputPane");
    setStyleSheet(R"(
        #outputPane    { background:#13151b; border-top:1px solid #1e2030; }
        QWidget#tabBar { background:#0d0e12; border-top:1px solid #1e2030; }
        QToolButton#paneTab {
            background:transparent; color:#8a95a3;
            border:none; padding:4px 12px;
            font-family:"Segoe UI"; font-size:11px;
            font-weight:600; letter-spacing:1px;
        }
        QToolButton#paneTab:hover     { color:#dce1e7; }
        QToolButton#paneTab:checked   { color:#5b8cff;
                                        background:#181a22;
                                        border-bottom:2px solid #5b8cff; }
        QToolButton#closeBtn {
            background:transparent; color:#6b7280; border:none;
            padding:4px 10px; font-size:14px; font-weight:bold;
        }
        QToolButton#closeBtn:hover { color:#ef4444; }
    )");

    auto *col = new QVBoxLayout(this);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    // Stacked panes (top)
    m_stack = new QStackedWidget(this);
    for (int i = 0; i < PaneCount; ++i) {
        auto *pe = makePane(m_stack);
        m_panes.append(pe);
        m_stack->addWidget(pe);
    }
    col->addWidget(m_stack, 1);

    // Tab bar (bottom)
    auto *tabBar = new QWidget(this);
    tabBar->setObjectName("tabBar");
    tabBar->setFixedHeight(28);
    auto *tabRow = new QHBoxLayout(tabBar);
    tabRow->setContentsMargins(8, 0, 6, 0);
    tabRow->setSpacing(2);

    const QStringList labels = {
        "1  ISSUES", "2  SEARCH", "3  APPLICATION OUTPUT",
        "4  COMPILE OUTPUT", "5  DEBUGGER CONSOLE"
    };
    for (int i = 0; i < labels.size(); ++i) {
        auto *btn = makeTabButton(i, labels[i]);
        m_tabs.append(btn);
        tabRow->addWidget(btn);
    }
    tabRow->addStretch();
    m_closeBtn = new QToolButton(tabBar);
    m_closeBtn->setObjectName("closeBtn");
    m_closeBtn->setText("×");
    m_closeBtn->setToolTip("Hide output pane");
    connect(m_closeBtn, &QToolButton::clicked, this, &OutputPane::onCloseClicked);
    tabRow->addWidget(m_closeBtn);

    col->addWidget(tabBar, 0);

    // Alt+1 .. Alt+5 shortcuts to switch panes
    for (int i = 0; i < PaneCount; ++i) {
        auto *sc = new QShortcut(QKeySequence(QString("Alt+%1").arg(i + 1)), this);
        connect(sc, &QShortcut::activated, this, [this, i]{ show(static_cast<Pane>(i)); });
    }

    show(static_cast<Pane>(m_current));
}

QToolButton *OutputPane::makeTabButton(int idx, const QString &label) {
    auto *b = new QToolButton(this);
    b->setObjectName("paneTab");
    b->setText(label);
    b->setCheckable(true);
    b->setAutoExclusive(false);
    connect(b, &QToolButton::clicked, this, [this, idx]{ onTabClicked(idx); });
    return b;
}

void OutputPane::onTabClicked(int pane) {
    // Clicking the active button toggles content visibility.
    if (pane == m_current && m_contentVisible) {
        hideContent();
        return;
    }
    show(static_cast<Pane>(pane));
}

void OutputPane::onCloseClicked() {
    hideContent();
}

void OutputPane::show(Pane pane) {
    m_current = pane;
    m_stack->setCurrentIndex(pane);
    if (!m_contentVisible) {
        m_stack->setVisible(true);
        m_contentVisible = true;
        emit visibilityChanged(true);
    }
    for (int i = 0; i < m_tabs.size(); ++i)
        m_tabs[i]->setChecked(i == pane);
}

void OutputPane::hideContent() {
    if (!m_contentVisible) return;
    m_stack->setVisible(false);
    m_contentVisible = false;
    for (auto *b : m_tabs) b->setChecked(false);
    emit visibilityChanged(false);
}

void OutputPane::appendTo(Pane pane, const QString &line, const QString &color) {
    if (pane < 0 || pane >= m_panes.size()) return;
    QString stamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString c = color.isEmpty() ? "#dce1e7" : color;
    m_panes[pane]->appendHtml(QString("<span style='color:#6b7280'>[%1]</span> "
                                      "<span style='color:%2'>%3</span>")
                                .arg(stamp, c, line.toHtmlEscaped()));
}
