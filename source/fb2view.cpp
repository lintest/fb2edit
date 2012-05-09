#include "fb2view.h"
#include "fb2read.h"

#include <QAction>
#include <QtDebug>
#include <QWebElement>
#include <QWebFrame>

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
    settings->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    settings->setAttribute(QWebSettings::JavaEnabled, false);
    settings->setAttribute(QWebSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
    settings->setAttribute(QWebSettings::PluginsEnabled, false);
    settings->setAttribute(QWebSettings::ZoomTextOnly, true);
    settings->setUserStyleSheetUrl(QUrl::fromLocalFile(":/res/style.css"));
    connect(page(), SIGNAL(contentsChanged()), this, SLOT(fixContents()));
}

Fb2WebView::~Fb2WebView()
{
    foreach (QString value, m_files) QFile::remove(value);
}

void Fb2WebView::fixContents()
{
    QWebElement doc = page()->mainFrame()->documentElement();
    while (true) {
        QWebElement span = doc.findFirst("span");
        if (span.isNull()) break;
        span.setOuterXml(span.toInnerXml());
    }
}

void Fb2WebView::load(const QString &filename)
{
    if (m_thread) return;
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

bool Fb2WebView::UndoEnabled()
{
    return pageAction(QWebPage::Undo)->isEnabled();
}

bool Fb2WebView::RedoEnabled()
{
    return pageAction(QWebPage::Redo)->isEnabled();
}

bool Fb2WebView::CutEnabled()
{
    return pageAction(QWebPage::Cut)->isEnabled();
}

bool Fb2WebView::CopyEnabled()
{
    return pageAction(QWebPage::Copy)->isEnabled();
}

bool Fb2WebView::BoldChecked()
{
    return pageAction(QWebPage::ToggleBold)->isChecked();
}

bool Fb2WebView::ItalicChecked()
{
    return pageAction(QWebPage::ToggleItalic)->isChecked();
}

bool Fb2WebView::StrikeChecked()
{
    return pageAction(QWebPage::ToggleStrikethrough)->isChecked();
}

bool Fb2WebView::SubChecked()
{
    return pageAction(QWebPage::ToggleSubscript)->isChecked();
}

bool Fb2WebView::SupChecked()
{
    return pageAction(QWebPage::ToggleSuperscript)->isChecked();
}
