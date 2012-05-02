#ifndef FB2VIEW_H
#define FB2VIEW_H

#include <QWebView>
#include <QNetworkAccessManager>

class Fb2NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit Fb2NetworkAccessManager(QObject *parent = 0);

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = 0);

};

class Fb2WebView : public QWebView
{
    Q_OBJECT
public:
    explicit Fb2WebView(QWidget *parent = 0);
    
signals:
    
public slots:
    void zoomIn();
    void zoomOut();
    void zoomOrig();

private:
    Fb2NetworkAccessManager m_network;

};

#endif // FB2VIEW_H
