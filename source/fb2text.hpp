#ifndef FB2TEXT_H
#define FB2TEXT_H

#include <QAction>
#include <QDockWidget>
#include <QFrame>
#include <QMainWindow>
#include <QResizeEvent>
#include <QTimer>
#include <QThread>
#include <QWebElement>
#include <QWebView>

#include "fb2enum.h"
#include "fb2temp.hpp"

QT_BEGIN_NAMESPACE
class QToolBar;
class QWebInspector;
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
    explicit FbTextEdit(QWidget *parent, QWidget *owner);
    virtual ~FbTextEdit();

    FbTextPage *page();
    FbNetworkAccessManager *files();
    bool save(QIODevice *device, const QString &codec = QString());
    bool save(QByteArray *array);
    bool save(QString *string);

    QAction * act(Fb::Actions index) const;
    void setAction(Fb::Actions index, QAction *action);
    void connectActions(QToolBar *tool);
    void disconnectActions();

    bool BoldChecked();
    bool ItalicChecked();
    bool StrikeChecked();
    bool SubChecked();
    bool SupChecked();

protected:
    virtual void mouseMoveEvent(QMouseEvent *event);

public slots:
    void html(QString html);
    void data(QString name, QByteArray data);
    void insertImage();
    void insertNote();
    void insertLink();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void find();

private slots:
    void linkHovered(const QString &link, const QString &title, const QString &textContent);
    void contextMenu(const QPoint &pos);
    void createImgs();
    void createTree();
    void treeDestroyed();
    void imgsDestroyed();

private:
    void viewTree();
    void viewImgs();
    void viewInsp();
    bool actionEnabled(QWebPage::WebAction action);
    bool actionChecked(QWebPage::WebAction action);
    void execCommand(const QString &cmd, const QString &arg);
    FbTemporaryFile * file(const QString &name);
    FbNoteView & noteView();
    QWebElement body();
    QWebElement doc();

private:
    QMainWindow *m_owner;
    FbNoteView *m_noteView;
    FbReadThread *m_thread;
    QMap<Fb::Actions, QAction*> m_actions;
    QDockWidget *dockTree;
    QDockWidget *dockImgs;
    QPoint m_point;
};

class FbWebFrame : public QFrame
{
    Q_OBJECT
public:
    explicit FbWebFrame(QWidget *parent = 0);
};

class FbTextFrame : public QFrame
{
    Q_OBJECT

public:
    explicit FbTextFrame(QWidget *parent, QAction *action);
    ~FbTextFrame();

public:
    FbTextEdit * view() { return &m_view; }

public slots:
    void showInspector();
    void hideInspector();

private slots:
    void dockDestroyed();

private:
    FbTextEdit m_view;
    QDockWidget *m_dock;
};

class FbTextAction : public QAction
{
    Q_OBJECT

public:
    explicit FbTextAction(const QString &text, QWebPage::WebAction action, QObject* parent);
    explicit FbTextAction(const QIcon &icon, const QString &text, QWebPage::WebAction action, QObject* parent);

public slots:
    void updateChecked();
    void updateEnabled();

private:
    QAction * action(QWebPage::WebAction action);

private:
    QWebPage::WebAction m_action;
};

#endif // FB2TEXT_H
