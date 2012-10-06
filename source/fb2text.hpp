#ifndef FB2TEXT_H
#define FB2TEXT_H

#include <QAction>
#include <QFrame>
#include <QResizeEvent>
#include <QTimer>
#include <QThread>
#include <QWebElement>
#include <QWebView>

#include "fb2temp.hpp"

QT_BEGIN_NAMESPACE
class QDockWidget;
class QToolBar;
class QUndoCommand;
class QWebInspector;
QT_END_NAMESPACE

class FbNoteView;
class FbReadThread;
class FbTextElement;

class FbTextLogger : public QObject
{
    Q_OBJECT

public:
    explicit FbTextLogger(QObject *parent = 0) : QObject(parent) {}

public slots:
    void trace(const QString &text);

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

class FbTextPage : public QWebPage
{
    Q_OBJECT

public:
    explicit FbTextPage(QObject *parent = 0);

    FbNetworkAccessManager *temp();
    void push(QUndoCommand * command, const QString &text = QString());
    FbTextElement element(const QString &location);
    FbTextElement current();
    QString location();
    QString status();

    FbTextElement body();
    FbTextElement doc();

    void appendSection(const FbTextElement &parent);

public slots:
    void insertBody();
    void insertTitle();
    void insertAnnot();
    void insertAuthor();
    void insertEpigraph();
    void insertSubtitle();
    void insertSection();
    void insertPoem();
    void insertStanza();
    void insertDate();
    void insertText();
    void createSection();
    void deleteSection();
    void createTitle();

protected:
    virtual bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);
    void createBlock(const QString &name);

protected:
    static QString block(const QString &name, const QString &text);
    static QString p(const QString &text = "<br/>");
    void update();

private slots:
    void loadFinished();
    void fixContents();

private:
    FbTextLogger m_logger;
};

class FbTextEdit : public FbTextBase
{
    Q_OBJECT

public:
    explicit FbTextEdit(QWidget *parent = 0);
    virtual ~FbTextEdit();

    FbTextPage *page();
    FbNetworkAccessManager *files();
    void load(const QString &filename, const QString &xml = QString());
    bool save(QIODevice *device, const QString &codec = QString());
    bool save(QByteArray *array);
    bool save(QString *string);

    bool actionEnabled(QWebPage::WebAction action);
    bool actionChecked(QWebPage::WebAction action);

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

private:
    void execCommand(const QString &cmd, const QString &arg);
    void exec(QUndoCommand *command);
    FbTemporaryFile * file(const QString &name);
    FbNoteView & noteView();
    QWebElement body();
    QWebElement doc();

private:
    FbNoteView *m_noteView;
    FbReadThread *m_thread;
    QPoint m_point;
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
