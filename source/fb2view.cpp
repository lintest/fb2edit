#include "fb2view.h"
#include "fb2read.h"

#include <QAction>
#include <QtDebug>
#include <QNetworkRequest>
#include <QToolTip>
#include <QWebElement>
#include <QWebFrame>
#include <QWebPage>

//---------------------------------------------------------------------------
//  Fb2WebPage
//---------------------------------------------------------------------------

Fb2WebPage::Fb2WebPage(QObject *parent)
    : QWebPage(parent)
{
    setContentEditable(true);
    QWebSettings *s = settings();
    s->setAttribute(QWebSettings::AutoLoadImages, true);
    s->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    s->setAttribute(QWebSettings::JavaEnabled, false);
    s->setAttribute(QWebSettings::JavascriptEnabled, true);
    s->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
    s->setAttribute(QWebSettings::PluginsEnabled, false);
    s->setAttribute(QWebSettings::ZoomTextOnly, true);
    s->setUserStyleSheetUrl(QUrl::fromLocalFile(":style.css"));
}

bool Fb2WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    Q_UNUSED(frame);
    if (type == NavigationTypeLinkClicked) {
        qCritical() << request.url().fragment();
//        QToolTip::showText(request.url().fragment());
    }
    return QWebPage::acceptNavigationRequest(frame, request, type);
}

//---------------------------------------------------------------------------
//  Fb2WebView
//---------------------------------------------------------------------------

Fb2WebView::Fb2WebView(QWidget *parent)
    : Fb2BaseWebView(parent)
    , m_thread(0)
{
    setPage(new Fb2WebPage(this));
    connect(page(), SIGNAL(contentsChanged()), this, SLOT(fixContents()));
    connect(page(), SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString,QString,QString)));
}

Fb2WebView::~Fb2WebView()
{
    foreach (QString value, m_files) QFile::remove(value);
}

QWebElement Fb2WebView::doc()
{
    return page()->mainFrame()->documentElement();
}

QString Fb2WebView::toXml()
{
    return doc().toOuterXml();
}

QString Fb2WebView::toBodyXml()
{
    QWebElement child = doc().firstChild();
    while (!child.isNull()) {
        if (child.tagName().toLower() == "body") {
            return child.toOuterXml();
        }
    }
    return QString();
}

void Fb2WebView::fixContents()
{
    foreach (QWebElement span, doc().findAll("span.apple-style-span[style]")) {
        span.removeAttribute("style");
    }
}

void Fb2WebView::linkHovered(const QString &link, const QString &title, const QString &textContent)
{
    QToolTip::showText(QPoint(100, 100), link);
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
