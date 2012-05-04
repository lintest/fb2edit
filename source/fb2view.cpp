#include "fb2view.h"
#include "fb2read.h"

#include <QtDebug>

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
}

Fb2WebView::~Fb2WebView()
{
    foreach (QString value, m_files) QFile::remove(value);
}

bool Fb2WebView::load(const QString &filename)
{
    if (m_thread) return false;
    m_thread = new Fb2ReadThread(this, filename);
    connect(m_thread, SIGNAL(file(QString, QString)), SLOT(file(QString, QString)));
    connect(m_thread, SIGNAL(html(QString, QString)), SLOT(html(QString, QString)));
    m_thread->start();
}

void Fb2WebView::file(QString name, QString path)
{
    m_files.insert(name, path);
}

void Fb2WebView::html(QString name, QString html)
{
    setHtml(html, QUrl::fromLocalFile(name));
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
