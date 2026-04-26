// =============================================================================
// OutputPane — Qt Creator-style bottom panel.
//
//   ┌─────────────────────────────────────────────────────────────────┐
//   │                                                                 │
//   │      Active output content (QPlainTextEdit, switchable)         │
//   │                                                                 │
//   ├─────────────────────────────────────────────────────────────────┤
//   │  1 Issues  2 Search  3 App Output  4 Compile  5 Debug   ▾  ✕    │
//   └─────────────────────────────────────────────────────────────────┘
//
// Each numbered button toggles a different output pane on top.  Clicking
// the active button hides the content area entirely (just leaving the
// button strip).  Alt+1..5 switches panes from anywhere in the window.
// =============================================================================
#ifndef NEXOR_STUDIO_OUTPUTPANE_H
#define NEXOR_STUDIO_OUTPUTPANE_H

#include <QWidget>
#include <QVector>

class QStackedWidget;
class QPlainTextEdit;
class QToolButton;
class QHBoxLayout;

class OutputPane : public QWidget {
    Q_OBJECT
public:
    enum Pane {
        PaneIssues       = 0,
        PaneSearch       = 1,
        PaneAppOutput    = 2,
        PaneCompile      = 3,
        PaneDebugConsole = 4,
        PaneCount
    };

    explicit OutputPane(QWidget *parent = nullptr);

    void appendTo(Pane pane, const QString &line, const QString &color = QString());
    void show(Pane pane);
    void hideContent();

signals:
    void visibilityChanged(bool contentVisible);

private slots:
    void onTabClicked(int pane);
    void onCloseClicked();

private:
    QToolButton *makeTabButton(int idx, const QString &label);

    QStackedWidget         *m_stack;
    QVector<QPlainTextEdit*> m_panes;
    QVector<QToolButton*>    m_tabs;
    QToolButton            *m_closeBtn;
    int                     m_current { PaneCompile };
    bool                    m_contentVisible { true };
};

#endif // NEXOR_STUDIO_OUTPUTPANE_H
