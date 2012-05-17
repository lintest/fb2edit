#ifndef FB2VIEW_H
#define FB2VIEW_H

#include <QByteArray>
#include <QHash>
#include <QResizeEvent>
#include <QTemporaryFile>
#include <QTimer>
#include <QThread>
#include <QWebElement>
#include <QWebView>

class Fb2BaseWebView : public QWebView
{
    Q_OBJECT

public:
    Fb2BaseWebView(QWidget* parent = 0)
        : QWebView(parent), m_empty(true)
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
          if (m_empty) return QWebView::resizeEvent(event);
          if (!m_timer.isActive()) m_size = event->oldSize();
          m_timer.start();
     }

protected:
    bool m_empty;

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
    void load(const QString &filename);
    bool save(const QString &filename);
    bool save(QIODevice &device);
    QString fileName(const QString &path);
    QString fileData(const QString &name);
    QString toBodyXml();
    QString toXml();

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
    void zoomIn();
    void zoomOut();
    void zoomOrig();

private slots:
    void fixContents();

private:
    QTemporaryFile * file(const QString &name);
    QWebElement doc();

private:
    typedef QHash<QString, QTemporaryFile*> TemporaryHash;
    TemporaryHash m_files;
    QThread *m_thread;
};

#endif // FB2VIEW_H
