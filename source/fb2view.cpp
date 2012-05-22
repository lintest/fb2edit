#include "fb2view.h"
#include "fb2read.h"
#include "fb2save.h"
#include "fb2xml2.h"

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
        return false;
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
    foreach (QTemporaryFile *file, m_files) delete file;
}

QWebElement Fb2WebView::doc()
{
    return page()->mainFrame()->documentElement();
}

QString Fb2WebView::toBodyXml()
{
    QWebElement body = doc().findFirst("body");
    if (body.isNull()) return QString();
    return body.toOuterXml();
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

void Fb2WebView::load(const QString &filename, const QString &xml)
{
    if (m_thread) return;
    m_thread = new Fb2ReadThread(this, filename, xml);
    m_thread->start();
}

bool Fb2WebView::save(QIODevice *device)
{
    Fb2SaveHandler handler(*this, device);
    QXmlInputSource source;
    source.setData(toBodyXml());
    XML2::HtmlReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    return reader.parse(source);
}

bool Fb2WebView::save(QString *string)
{
    // Use class QByteArray instead QString
    // to store information about encoding.
    QByteArray data;
    Fb2SaveHandler handler(*this, &data);
    QXmlInputSource source;
    source.setData(toBodyXml());
    XML2::HtmlReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    bool ok = reader.parse(source);
    if (ok) *string = QString::fromUtf8(data.data());
    return ok;
}

QTemporaryFile * Fb2WebView::file(const QString &name)
{
    TemporaryHash::const_iterator it = m_files.find(name);
    if (it == m_files.end()) {
        it = m_files.insert(name, new QTemporaryFile());
        it.value()->open();
    }
    return it.value();
}

QString Fb2WebView::temp(QString name)
{
    return file(name)->fileName();
}

void Fb2WebView::data(QString name, QByteArray data)
{
    QTemporaryFile * temp = file(name);
    temp->write(data);
    temp->close();
    temp->open();
}

QString Fb2WebView::fileName(const QString &path)
{
    QHashIterator<QString, QTemporaryFile*> it(m_files);
    while (it.hasNext()) {
        it.next();
        if (it.value()->fileName() == path) return it.key();
    }
    return QString();
}

QString Fb2WebView::fileData(const QString &name)
{
    QTemporaryFile * temp = file(name);
    return temp->readAll().toBase64();
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

