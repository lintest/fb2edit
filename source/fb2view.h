#ifndef FB2VIEW_H
#define FB2VIEW_H

#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QResizeEvent>
#include <QTimer>
#include <QThread>
#include <QWebView>

class Fb2ImageReply : public QNetworkReply
{
    Q_OBJECT
public:
    explicit Fb2ImageReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request, const QByteArray &data);
    void abort();
    qint64 bytesAvailable() const;
    bool isSequential() const;

protected:
    qint64 readData(char *data, qint64 maxSize);

private:
    QByteArray content;
    qint64 offset;
};

class Fb2NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit Fb2NetworkAccessManager(QObject *parent = 0);
    void insert(const QString &file, const QByteArray &data);

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = 0);

private:
    QNetworkReply *imageRequest(Operation op, const QNetworkRequest &request);

private:
    typedef QMap<QString, QByteArray> ImageMap;
    ImageMap m_images;

};

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

class Fb2WebView : public Fb2BaseWebView
{
    Q_OBJECT
public:
    explicit Fb2WebView(QWidget *parent = 0);
    bool load(const QString &filename);
    
signals:
    
public slots:
    void image(QString file, QByteArray data);
    void html(QString html);
    void zoomIn();
    void zoomOut();
    void zoomOrig();

private:
    Fb2NetworkAccessManager m_network;
    QThread *m_thread;
};

#endif // FB2VIEW_H
