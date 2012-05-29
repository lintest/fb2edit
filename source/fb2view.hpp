#ifndef FB2VIEW_H
#define FB2VIEW_H

#include <QResizeEvent>
#include <QTimer>
#include <QThread>
#include <QWebElement>
#include <QWebView>

#include "fb2temp.hpp"

class Fb2BaseWebView : public QWebView
{
    Q_OBJECT

public:
    Fb2BaseWebView(QWidget* parent = 0)
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

class Fb2WebPage : public QWebPage
{
    Q_OBJECT

public:
    explicit Fb2WebPage(QObject *parent = 0);

protected:
    virtual bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);
};

class Fb2WebView : public Fb2BaseWebView
{
    Q_OBJECT
public:
    explicit Fb2WebView(QWidget *parent = 0);
    virtual ~Fb2WebView();

    const Fb2TemporaryList & files() const { return m_files; }
    void load(const QString &filename, const QString &xml = QString());
    bool save(QByteArray *array, QList<int> *folds = 0);
    bool save(QIODevice *device);
    bool save(QString *string);
    QString toBodyXml();

    bool UndoEnabled();
    bool RedoEnabled();
    bool CutEnabled();
    bool CopyEnabled();
    bool BoldChecked();
    bool ItalicChecked();
    bool StrikeChecked();
    bool SubChecked();
    bool SupChecked();

signals:
    
public slots:
    QString temp(QString name);
    void data(QString name, QByteArray data);
    void html(QString name, QString html);
    void linkHovered(const QString &link, const QString &title, const QString &textContent);
    void insertImage();
    void zoomIn();
    void zoomOut();
    void zoomOrig();

private slots:
    void fixContents();

private:
    void execCommand(const QString &cmd, const QString &arg);
    Fb2TemporaryFile * file(const QString &name);
    QWebElement doc();

private:
    Fb2TemporaryList m_files;
    QThread *m_thread;
};

#endif // FB2VIEW_H
