#ifndef FB2VIEW_H
#define FB2VIEW_H

#include <QFrame>
#include <QResizeEvent>
#include <QTimer>
#include <QThread>
#include <QWebElement>
#include <QWebView>

#include "fb2temp.hpp"

QT_BEGIN_NAMESPACE
class QWebInspector;
class QDockWidget;
QT_END_NAMESPACE

class Fb2NoteView;
class Fb2TextElement;

class Fb2TextBase : public QWebView
{
    Q_OBJECT

public:
    Fb2TextBase(QWidget* parent = 0)
        : QWebView(parent)
    {
          m_timer.setInterval(100);
          m_timer.setSingleShot(true);
          connect(&m_timer, SIGNAL(timeout()), SLOT(doResize()));
    }

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

private:
    QTimer m_timer;
    QSize m_size;
};

class Fb2TextPage : public QWebPage
{
    Q_OBJECT

public:
    explicit Fb2TextPage(QObject *parent = 0);

    void update();
    Fb2TextElement element(const QString &location);
    Fb2TextElement current();
    QString location();
    QString status();

    Fb2TextElement body();
    Fb2TextElement doc();

public slots:
    void insertBody();
    void insertSubtitle();

protected:
    virtual bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);
};

class Fb2TextEdit : public Fb2TextBase
{
    Q_OBJECT

public:
    explicit Fb2TextEdit(QWidget *parent = 0);
    virtual ~Fb2TextEdit();

    Fb2TextPage * page();
    Fb2TemporaryList & files() { return m_files; }
    void load(const QString &filename, const QString &xml = QString());
    bool save(QIODevice *device, const QString &codec = QString());
    bool save(QByteArray *array);
    bool save(QString *string);

    bool UndoEnabled();
    bool RedoEnabled();
    bool CutEnabled();
    bool CopyEnabled();
    bool BoldChecked();
    bool ItalicChecked();
    bool StrikeChecked();
    bool SubChecked();
    bool SupChecked();

protected:
    virtual void mouseMoveEvent(QMouseEvent *event);

public slots:
    void data(QString name, QByteArray data);
    void html(QString name, QString html);
    void linkHovered(const QString &link, const QString &title, const QString &textContent);
    void insertImage();
    void insertNote();
    void insertLink();
    void insertTitle();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void find();

private slots:
    void fixContents();
    void loadFinished();

private:
    void execCommand(const QString &cmd, const QString &arg);
    Fb2TemporaryFile * file(const QString &name);
    Fb2NoteView & noteView();
    QWebElement body();
    QWebElement doc();

private:
    Fb2TemporaryList m_files;
    Fb2NoteView *m_noteView;
    QThread *m_thread;
    QPoint m_point;
};

class Fb2TextFrame : public QFrame
{
    Q_OBJECT

public:
    explicit Fb2TextFrame(QWidget* parent = 0);
    ~Fb2TextFrame();
    void hideInspector();
    Fb2TextEdit view;

public slots:
    void showInspector();

private:
    QDockWidget * dock;
};

#endif // FB2VIEW_H
