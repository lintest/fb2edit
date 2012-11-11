#ifndef FB2DOCK_H
#define FB2DOCK_H

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QStackedWidget>
#include <QToolBar>
#include <QIODevice>

#include "fb2mode.h"

class FbTextEdit;
class FbHeadEdit;
class FbCodeEdit;

class FbMainDock : public QStackedWidget
{
    Q_OBJECT

public:
    explicit FbMainDock(QWidget *parent = 0);
    FbTextEdit * text() { return m_text; }
    FbHeadEdit * head() { return m_head; }
    FbCodeEdit * code() { return m_code; }
    bool load(const QString &filename);
    bool save(QIODevice *device, const QString &codec = QString());
    Fb::Mode mode() const { return m_mode; }
    void switchMode(Fb::Mode mode);
    void setMode(Fb::Mode mode);
    void setTool(QToolBar *tool) { m_tool = tool; }
    void addAction(Fb::Mode mode, QAction *action);
    void addMenu(QMenu *menu);
    bool isModified() const;

signals:
    void status(const QString &text);

private slots:
    void error(int row, int col);

private:
    void enableMenu(bool value);
    void setModeText();
    void setModeHead();
    void setModeCode();
    void setModeHtml();

private:
    QFrame *textFrame;
    QMap<Fb::Mode, QAction*> m_actions;
    QList<QMenu*> m_menus;
    FbTextEdit *m_text;
    FbHeadEdit *m_head;
    FbCodeEdit *m_code;
    QToolBar *m_tool;
    bool isSwitched;
    Fb::Mode m_mode;
};

class FbModeAction : public QAction
{
    Q_OBJECT
public:
    explicit FbModeAction(FbMainDock *parent, Fb::Mode mode, const QIcon &icon, const QString &text);
    explicit FbModeAction(FbMainDock *parent, Fb::Mode mode, const QString &text);

private slots:
    void switchMode();

private:
    FbMainDock * m_dock;
    Fb::Mode m_mode;
};

#endif // FB2DOCK_H
