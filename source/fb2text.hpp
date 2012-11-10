#ifndef FB2TEXT_H
#define FB2TEXT_H

#include <QAction>
#include <QDockWidget>
#include <QFrame>
#include <QResizeEvent>
#include <QTimer>
#include <QWebElement>
#include <QWebView>

#include "fb2mode.h"
#include "fb2imgs.hpp"

QT_BEGIN_NAMESPACE
class QMainWindow;
class QToolBar;
QT_END_NAMESPACE

class FbNoteView;
class FbReadThread;
class FbTextPage;

class FbDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit FbDockWidget(const QString &title, QWidget *parent = 0, Qt::WindowFlags flags = 0);
};

class FbTextBase : public QWebView
{
    Q_OBJECT

public:
    FbTextBase(QWidget *parent = 0)
        : QWebView(parent)
    {
          m_timer.setInterval(100);
          m_timer.setSingleShot(true);
          connect(&m_timer, SIGNAL(timeout()), SLOT(doResize()));
    }

    void addTools(QToolBar *tool);

protected slots:
    void doResize() {
        QResizeEvent event(size(), m_size);
        QWebView::resizeEvent(&event);
        QWebView::update();
    }

protected:
     void resizeEvent(QResizeEvent* event) {
          if (!m_timer.isActive()) m_size = event->oldSize();
          m_timer.start();
     }

     void keyPressEvent(QKeyEvent *event) {
         if (event->key() == Qt::Key_Escape) return;
         QWebView::keyPressEvent(event);
     }

private:
    QTimer m_timer;
    QSize m_size;
};

class FbTextEdit : public FbTextBase
{
    Q_OBJECT

public:
    explicit FbTextEdit(QWidget *parent, QObject *owner);
    virtual ~FbTextEdit();

    FbTextPage *page();
    FbStore *store();
    bool save(QIODevice *device, const QString &codec = QString());
    bool save(QString *string, int &anchor, int &focus);
    bool save(QByteArray *array);
    QString toHtml();

    QAction * act(Fb::Actions index) const;
    QAction * pAct(QWebPage::WebAction index) const;
    void setAction(Fb::Actions index, QAction *action);
    void connectActions(QToolBar *tool);
    void disconnectActions();
    void hideDocks();

    bool BoldChecked();
    bool ItalicChecked();
    bool StrikeChecked();
    bool SubChecked();
    bool SupChecked();

    QWebElement body();
    QWebElement doc();

protected:
    virtual void mouseMoveEvent(QMouseEvent *event);

public slots:
    void viewContents(bool show);
    void viewPictures(bool show);
    void viewFootnotes(bool show);
    void viewInspector(bool show);
    void insertImage();
    void insertNote();
    void insertLink();
    void find();

private slots:
    void linkHovered(const QString &link, const QString &title, const QString &textContent);
    void contextMenu(const QPoint &pos);
    void treeDestroyed();
    void imgsDestroyed();
    void noteDestroyed();
    void zoomIn();
    void zoomOut();
    void zoomReset();

private:
    bool actionEnabled(QWebPage::WebAction action);
    bool actionChecked(QWebPage::WebAction action);
    void execCommand(const QString &cmd, const QString &arg);
    FbBinary * file(const QString &name);
    FbNoteView & noteView();

private:
    QMainWindow *m_owner;
    FbNoteView *m_noteView;
    FbReadThread *m_thread;
    FbActionMap m_actions;
    QDockWidget *dockTree;
    QDockWidget *dockNote;
    QDockWidget *dockImgs;
    QDockWidget *dockInsp;
    QPoint m_point;
};

class FbTextFrame : public QFrame
{
    Q_OBJECT
public:
    explicit FbTextFrame(QWidget *parent = 0);
};

class FbTextAction : public QAction
{
    Q_OBJECT

public:
    explicit FbTextAction(const QString &text, QWebPage::WebAction action, FbTextEdit* parent);
    explicit FbTextAction(const QIcon &icon, const QString &text, QWebPage::WebAction action, FbTextEdit *parent);
    void connectAction();
    void disconnectAction();

private slots:
    void updateAction();

private:
    QAction * action(QWebPage::WebAction action);

private:
    QWebPage::WebAction m_action;
    FbTextEdit *m_parent;
};

#endif // FB2TEXT_H
