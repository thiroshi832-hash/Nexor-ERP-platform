// =============================================================================
// FancyTabBar — vertical mode picker styled like Qt Creator's left tab bar.
//
// Each tab is a custom-painted square with a drawn icon and a label below it.
// The active tab gets a solid color background, an accent stripe on the left
// edge, and a brighter foreground.
// =============================================================================
#ifndef NEXOR_STUDIO_FANCYTABBAR_H
#define NEXOR_STUDIO_FANCYTABBAR_H

#include <QWidget>
#include <QVector>

// ────────────────────────────────────────────────────────────────────────────
// FancyTab — single button in the bar.  Lives in the header so qmake's
// standard MOC pass sees its Q_OBJECT.
// ────────────────────────────────────────────────────────────────────────────
class FancyTab : public QWidget {
    Q_OBJECT
public:
    FancyTab(int mode, const QString &iconKind, const QString &label, QWidget *parent = nullptr);

    int  mode() const     { return m_mode; }
    bool isActive() const { return m_active; }
    void setActive(bool a);

signals:
    void clicked(int mode);

protected:
    QSize sizeHint() const override;
    void  enterEvent(QEvent *) override;
    void  leaveEvent(QEvent *) override;
    void  mousePressEvent(QMouseEvent *e) override;
    void  paintEvent(QPaintEvent *) override;

private:
    void drawIcon(class QPainter &p, const QRect &r, const QColor &c);

    int     m_mode;
    QString m_iconKind;
    QString m_label;
    bool    m_active { false };
    bool    m_hover  { false };
};

// ────────────────────────────────────────────────────────────────────────────
// FancyTabBar
// ────────────────────────────────────────────────────────────────────────────
class FancyTabBar : public QWidget {
    Q_OBJECT
public:
    enum Mode {
        ModeWelcome  = 0,
        ModeEdit     = 1,
        ModeDesign   = 2,
        ModeDebug    = 3,
        ModeProjects = 4,
        ModeHelp     = 5
    };

    explicit FancyTabBar(QWidget *parent = nullptr);

    int  currentMode() const { return m_current; }
    void setCurrentMode(int mode);

signals:
    void currentChanged(int mode);

private:
    void addTab(int mode, const QString &iconKind, const QString &label);

    QVector<FancyTab*> m_tabs;
    int                m_current { 0 };
};

#endif // NEXOR_STUDIO_FANCYTABBAR_H
