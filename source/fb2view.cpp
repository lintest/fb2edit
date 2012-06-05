#include "fb2view.hpp"
#include "fb2note.hpp"
#include "fb2read.hpp"
#include "fb2save.hpp"
#include "fb2utils.h"
#include "fb2xml2.h"

#include <QAction>
#include <QtDebug>
#include <QFileDialog>
#include <QNetworkRequest>
#include <QToolTip>
#include <QWebElement>
#include <QWebInspector>
#include <QWebFrame>
#include <QWebPage>

//---------------------------------------------------------------------------
//  Fb2NoteView
//---------------------------------------------------------------------------

class Fb2NoteView : public QWebView
{
public:
    explicit Fb2NoteView(QWidget* parent = 0) : QWebView(parent) {}
    void hint(const QWebElement element, const QRect &rect);
protected:
    void paintEvent(QPaintEvent *event);
};

void Fb2NoteView::paintEvent(QPaintEvent *event)
{
    QWebView::paintEvent(event);
    QPainter painter(this);
    painter.setPen(Qt::black);
    QSize size = geometry().size() - QSize(1, 1);
    painter.drawRect( QRect(QPoint(0, 0), size) );
}

void Fb2NoteView::hint(const QWebElement element, const QRect &rect)
{
    QString html = element.toOuterXml();
    html.prepend(
        "<body bgcolor=lightyellow style='overflow:hidden;padding:0;margin:0;margin-top:2;'>"
        "<div class=body name=notes style='padding:0;margin:0;'>"
    );
    html.append("</div></body>");
    setGeometry(rect);
    setHtml(html);
    show();
}

//---------------------------------------------------------------------------
//  Fb2WebPage
//---------------------------------------------------------------------------

Fb2WebPage::Fb2WebPage(QObject *parent)
    : QWebPage(parent)
{
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
    , m_inspector(0)
    , m_noteView(0)
    , m_thread(0)
{
    setPage(new Fb2WebPage(this));
    page()->setNetworkAccessManager(new Fb2NetworkAccessManager(*this));
    page()->setContentEditable(true);
    connect(page(), SIGNAL(contentsChanged()), this, SLOT(fixContents()));
    connect(page(), SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString,QString,QString)));
}

Fb2WebView::~Fb2WebView()
{
    FB2DELETE(m_inspector);
    FB2DELETE(m_noteView);
}

Fb2NoteView & Fb2WebView::noteView()
{
    if (m_noteView) return *m_noteView;
    m_noteView = new Fb2NoteView(qobject_cast<QWidget*>(parent()));
    m_noteView->setPage(new Fb2WebPage(this));
    m_noteView->page()->setNetworkAccessManager(page()->networkAccessManager());
    m_noteView->page()->setContentEditable(false);
    m_noteView->setGeometry(QRect(100, 100, 400, 200));
    return *m_noteView;
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

void Fb2WebView::mouseMoveEvent(QMouseEvent *event)
{
    m_point = event->pos();
    QWebView::mouseMoveEvent(event);
}

void Fb2WebView::linkHovered(const QString &link, const QString &title, const QString &textContent)
{
    Q_UNUSED(title);
    Q_UNUSED(textContent);

    const QString href = QUrl(link).fragment();
    if (href.isEmpty()) {
        if (m_noteView) m_noteView->hide();
        return;
    }

    const QString query = QString("DIV#%1").arg(href);
    const QWebElement element = doc().findFirst(query);
    if (element.isNull()) {
        if (m_noteView) m_noteView->hide();
        return;
    }

    QRect rect = geometry();
    QSize size = element.geometry().size() + QSize(2, 4);
    int center = rect.size().height() / 2;
    int h = size.height();
    if (h > center) size.setHeight(center - 10);
    int x = (rect.size().width() - size.width()) / 2;
    int y = m_point.y();
    if ( y > h ) y = y - h - 10; else y = y + 10;
    QPoint point = QPoint(x, y) + rect.topLeft();
    noteView().hint(element, QRect(point, size));
}

void Fb2WebView::load(const QString &filename, const QString &xml)
{
    if (m_thread) return;
    m_thread = new Fb2ReadThread(this, filename, xml);
    m_thread->start();
}

bool Fb2WebView::save(QIODevice *device)
{
    return Fb2SaveHandler(*this, device).save();
}

bool Fb2WebView::save(QByteArray *array, QList<int> *folds)
{
    return Fb2SaveHandler(*this, array, folds).save();
}

bool Fb2WebView::save(QString *string)
{
    // Use class QByteArray instead QString
    // to store information about encoding.
    QByteArray data;
    bool ok = save(&data);
    if (ok) *string = QString::fromUtf8(data.data());
    return ok;
}

QString Fb2WebView::temp(QString name)
{
    return m_files.get(name).fileName();
}

void Fb2WebView::data(QString name, QByteArray data)
{
    m_files.set(name, data);
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

void Fb2WebView::insertImage()
{
    QString filters;
    filters += tr("Common Graphics (*.png *.jpg *.jpeg *.gif);;");
    filters += tr("Portable Network Graphics (PNG) (*.png);;");
    filters += tr("JPEG (*.jpg *.jpeg);;");
    filters += tr("Graphics Interchange Format (*.gif);;");
    filters += tr("All Files (*)");

    QString fn = QFileDialog::getOpenFileName(this, tr("Open image..."), QString(), filters);
    if (fn.isEmpty()) return;
    if (!QFile::exists(fn)) return;

    QUrl url = QUrl::fromLocalFile(fn);
    execCommand("insertImage", url.toString());
}

void Fb2WebView::insertNote()
{
    Fb2NoteDlg dlg(*this);
    dlg.exec();
}


void Fb2WebView::insertLink()
{
    Fb2SaveDialog dlg;
    dlg.exec();
}

void Fb2WebView::execCommand(const QString &cmd, const QString &arg)
{
    QString javascript = QString("document.execCommand(\"%1\",false,\"%2\")").arg(cmd).arg(arg);
    page()->mainFrame()->evaluateJavaScript(javascript);
}

QString Fb2WebView::status()
{
    static const QString javascript = FB2::read(":/js/get_status.js");
    return page()->mainFrame()->evaluateJavaScript(javascript).toString();
    return QString();
}

void Fb2WebView::showInspector()
{
    if (!m_inspector) {
        m_inspector = new QWebInspector();
        m_inspector->setAttribute(Qt::WA_DeleteOnClose, false);
        m_inspector->setPage(page());
    }
    m_inspector->show();
}

