#include "fb2view.h"
#include "fb2read.h"

#include <QtDebug>
#include <QNetworkReply>
#include <QNetworkRequest>

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
    qCritical() << request.url().toString();
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
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
