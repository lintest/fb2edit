#include "fb2view.h"
#include "fb2read.h"

#include <QtDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QWebSecurityOrigin>

//---------------------------------------------------------------------------
//  Fb2ImageReply
//---------------------------------------------------------------------------

Fb2ImageReply::Fb2ImageReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request, const QByteArray &data)
    : QNetworkReply()
    , content(data)
    , offset(0)
{
/*
    QString path = "/home/user/tmp" + request.url().path();
    qCritical() << path;
    QFile file(path);
    file.open(QFile::WriteOnly);
    file.write(content);
    file.close();
*/
    qCritical() << tr("Image: %1 - %2").arg(request.url().toString()).arg(content.size());
    setRequest(request);
    setUrl(request.url());
    setOperation(op);
    open(ReadOnly | Unbuffered);
    setHeader(QNetworkRequest::ContentLengthHeader, QVariant(content.size()));
    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}

void Fb2ImageReply::abort()
{
    close();
}

qint64 Fb2ImageReply::bytesAvailable() const
{
    return content.size() - offset;
}

bool Fb2ImageReply::isSequential() const
{
    return true;
}

qint64 Fb2ImageReply::readData(char *data, qint64 maxSize)
{
    if (offset < content.size()) {
        qint64 number = qMin(maxSize, content.size() - offset);
        memcpy(data, content.constData() + offset, number);
        offset += number;
        return number;
    } else {
        return -1;
    }
}

//---------------------------------------------------------------------------
//  Fb2NetworkAccessManager
//
//    http://doc.trolltech.com/qq/32/qq32-webkit-protocols.html
//---------------------------------------------------------------------------

Fb2NetworkAccessManager::Fb2NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
}

QNetworkReply * Fb2NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    if (request.url().scheme() == "fb2") return imageRequest(op, request);
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

QNetworkReply * Fb2NetworkAccessManager::imageRequest(Operation op, const QNetworkRequest &request)
{
    QString file = request.url().path();
    while (file.left(1) == "/") file.remove(0, 1);
    ImageMap::const_iterator i = m_images.find(file);
    if (i == m_images.end()) {
        qCritical() << "Not found: " << file;
        return 0;
    } else {
        return new Fb2ImageReply(op, request, i.value());
    }
}

void Fb2NetworkAccessManager::insert(const QString &file, const QByteArray &data)
{
    m_images.insert(file, data);
}

//---------------------------------------------------------------------------
//  Fb2WebView
//---------------------------------------------------------------------------

Fb2WebView::Fb2WebView(QWidget *parent)
    : Fb2BaseWebView(parent)
    , m_thread(0)
{
    page()->setContentEditable(true);
    QWebSettings *settings = page()->settings();
    settings->setAttribute(QWebSettings::AutoLoadImages, true);
    settings->setAttribute(QWebSettings::JavaEnabled, false);
    settings->setAttribute(QWebSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
    settings->setAttribute(QWebSettings::PluginsEnabled, false);
    settings->setAttribute(QWebSettings::ZoomTextOnly, true);
    page()->setNetworkAccessManager(&m_network);
}

bool Fb2WebView::load(const QString &filename)
{
    if (m_thread) return false;
    m_thread = new Fb2ReadThread(this, filename);
    connect(m_thread, SIGNAL(image(QString, QByteArray)), SLOT(image(QString, QByteArray)));
    connect(m_thread, SIGNAL(html(QString)), SLOT(html(QString)));
    m_thread->start();
}

void Fb2WebView::image(QString file, QByteArray data)
{
    m_network.insert(file, data);
    qCritical() << file;
}

void Fb2WebView::html(QString html)
{
    setHtml(html, QUrl("fb2://s/"));
    if (m_thread) m_thread->deleteLater();
    m_thread = 0;
}

void Fb2WebView::zoomIn()
{
    qreal zoom = zoomFactor();
    setZoomFactor(zoom * 1.1);
}

void Fb2WebView::zoomOut()
{
    qreal zoom = zoomFactor();
    setZoomFactor(zoom * 0.9);
}

void Fb2WebView::zoomOrig()
{
    setZoomFactor(1);
}
