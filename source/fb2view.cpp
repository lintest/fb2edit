#include "fb2view.hpp"
#include "fb2dlgs.hpp"
#include "fb2read.hpp"
#include "fb2save.hpp"
#include "fb2utils.h"
#include "fb2html.h"
#include "fb2xml2.h"

#include <QAction>
#include <QtDebug>
#include <QFileDialog>
#include <QNetworkRequest>
#include <QStyle>
#include <QStyleOptionFrame>
#include <QToolTip>
#include <QUndoCommand>
#include <QUndoStack>
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
    explicit Fb2NoteView(QWidget *parent, const QUrl &url);
    void hint(const QWebElement element, const QRect &rect);
protected:
    void paintEvent(QPaintEvent *event);
    const QUrl m_url;
};

Fb2NoteView::Fb2NoteView(QWidget *parent, const QUrl &url)
    : QWebView(parent)
    , m_url(url)
{
}

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
        "<div class=body fb2_name=notes style='padding:0;margin:0;'>"
    );
    html.append("</div></body>");
    setGeometry(rect);
    setHtml(html, m_url);
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

QWebElement Fb2WebPage::body()
{
    return doc().findFirst("body");
}

QWebElement Fb2WebPage::doc()
{
    return mainFrame()->documentElement();
}

class Fb2InsertBodyCommand : public QUndoCommand
{
public:
    explicit Fb2InsertBodyCommand(Fb2WebPage &page, QUndoCommand *parent = 0) : QUndoCommand(parent), m_page(page) {}
    virtual void undo();
    virtual void redo();
private:
    Fb2WebPage & m_page;
};

void Fb2InsertBodyCommand::undo()
{
    m_page.body().lastChild().removeFromDocument();
    Fb2WebElement(m_page.body().lastChild()).select();
}

void Fb2InsertBodyCommand::redo()
{
    m_page.body().appendInside("<div class=body><div class=section><p>text</p></div></div>");
    Fb2WebElement(m_page.body().lastChild()).select();
}

void Fb2WebPage::insertBody()
{
    undoStack()->beginMacro("Insert title");
    undoStack()->push(new Fb2InsertBodyCommand(*this));
    undoStack()->endMacro();
    emit contentsChanged();
}

//---------------------------------------------------------------------------
//  Fb2BaseWebView
//---------------------------------------------------------------------------

void Fb2BaseWebView::paintEvent(QPaintEvent *event)
{
    QWebView::paintEvent(event);
    QPainter painter(this);
    QStyleOptionFrame option;
    option.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Frame, &option, &painter, this);
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
    connect(this, SIGNAL(loadFinished(bool)), SLOT(loadFinished()));
}

Fb2WebView::~Fb2WebView()
{
    FB2DELETE(m_inspector);
    FB2DELETE(m_noteView);
}

Fb2NoteView & Fb2WebView::noteView()
{
    if (m_noteView) return *m_noteView;
    m_noteView = new Fb2NoteView(qobject_cast<QWidget*>(parent()), url());
    m_noteView->setPage(new Fb2WebPage(this));
    m_noteView->page()->setNetworkAccessManager(page()->networkAccessManager());
    m_noteView->page()->setContentEditable(false);
    m_noteView->setGeometry(QRect(100, 100, 400, 200));
    return *m_noteView;
}

QWebElement Fb2WebView::body()
{
    return doc().findFirst("body");
}

QWebElement Fb2WebView::doc()
{
    return page()->mainFrame()->documentElement();
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

bool Fb2WebView::save(QIODevice *device, const QString &codec)
{
    Fb2SaveWriter writer(*this, device);
    if (!codec.isEmpty()) writer.setCodec(codec.toLatin1());
    bool ok = Fb2SaveHandler(writer).save();
    if (ok) page()->undoStack()->setClean();
    return ok;
}

bool Fb2WebView::save(QByteArray *array)
{
    Fb2SaveWriter writer(*this, array);
    return Fb2SaveHandler(writer).save();
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

void Fb2WebView::data(QString name, QByteArray data)
{
    m_files.set(name, data);
}

void Fb2WebView::html(QString name, QString html)
{
    static int number = 0;
    setHtml(html, QUrl(QString("fb2:/%1/").arg(number++)));
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

void Fb2WebView::zoomReset()
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

void Fb2WebView::find()
{
    Fb2TextFindDlg dlg(*this);
    dlg.exec();
}

void Fb2WebView::insertImage()
{
    QString filters;
    filters += tr("Common Graphics (*.png *.jpg *.jpeg *.gif);;");
    filters += tr("Portable Network Graphics (PNG) (*.png);;");
    filters += tr("JPEG (*.jpg *.jpeg);;");
    filters += tr("Graphics Interchange Format (*.gif);;");
    filters += tr("All Files (*)");

    QString path = QFileDialog::getOpenFileName(this, tr("Insert image..."), QString(), filters);
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    QString name = m_files.add(path, data);
    execCommand("insertImage", name.prepend("#"));
}

void Fb2WebView::insertNote()
{
    Fb2NoteDlg dlg(*this);
    dlg.exec();
}


void Fb2WebView::insertLink()
{
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

void Fb2WebView::loadFinished()
{
    Fb2WebElement element = body().findFirst("div.body");
    if (element.isNull()) element = body();
    element.select();
}

void Fb2WebView::insertTitle()
{
    page()->undoStack()->beginMacro("Insert title");
    static const QString javascript = FB2::read(":/js/insert_title.js");
    page()->mainFrame()->evaluateJavaScript(javascript);
    page()->undoStack()->endMacro();
}

