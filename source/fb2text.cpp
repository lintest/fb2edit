#include "fb2text.hpp"
#include "fb2dlgs.hpp"
#include "fb2read.hpp"
#include "fb2save.hpp"
#include "fb2utils.h"
#include "fb2html.h"
#include "fb2xml2.h"

#include <QAction>
#include <QBoxLayout>
#include <QDockWidget>
#include <QFileDialog>
#include <QMainWindow>
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
#include <QtDebug>

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
//  Fb2TextPage
//---------------------------------------------------------------------------

Fb2TextPage::Fb2TextPage(QObject *parent)
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

bool Fb2TextPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    Q_UNUSED(frame);
    if (type == NavigationTypeLinkClicked) {
        qCritical() << request.url().fragment();
        return false;
//        QToolTip::showText(request.url().fragment());
    }
    return QWebPage::acceptNavigationRequest(frame, request, type);
}

Fb2TextElement Fb2TextPage::body()
{
    return doc().findFirst("body");
}

Fb2TextElement Fb2TextPage::doc()
{
    return mainFrame()->documentElement();
}

void Fb2TextPage::insertBody()
{
    undoStack()->beginMacro("Append body");
    undoStack()->push(new Fb2AddBodyCmd(*this));
    undoStack()->endMacro();
}

void Fb2TextPage::update()
{
    emit contentsChanged();
    emit selectionChanged();
}

void Fb2TextPage::insertSubtitle()
{
    Fb2TextElement element = current();
    while (!element.isNull()) {
        Fb2TextElement parent = element.parent();
        if (parent.isSection()) {
            Fb2TextElement previous = element.previousSibling();
            if (!previous.isNull()) element = previous;
            undoStack()->beginMacro("Insert subtitle");
            undoStack()->push(new Fb2SubtitleCmd(*this, element.location()));
            undoStack()->endMacro();
            break;
        }
        element = parent;
    }
}

Fb2TextElement Fb2TextPage::current()
{
    return element(location());
}

Fb2TextElement Fb2TextPage::element(const QString &location)
{
    QStringList list = location.split(",");
    QStringListIterator iterator(list);
    QWebElement result = doc();
    while (iterator.hasNext()) {
        QString str = iterator.next();
        int pos = str.indexOf("=");
        QString tag = str.left(pos);
        int key = str.mid(pos + 1).toInt();
        if (key < 0) break;
        result = result.firstChild();
        while (0 < key--) result = result.nextSibling();
    }
    return result;
}

QString Fb2TextPage::location()
{
    static const QString javascript = FB2::read(":/js/get_location.js").prepend("var element=document.getSelection().anchorNode;");
    return mainFrame()->evaluateJavaScript(javascript).toString();
}

QString Fb2TextPage::status()
{
    static const QString javascript = FB2::read(":/js/get_status.js");
    return mainFrame()->evaluateJavaScript(javascript).toString();
}

//---------------------------------------------------------------------------
//  Fb2TextEdit
//---------------------------------------------------------------------------

Fb2TextEdit::Fb2TextEdit(QWidget *parent)
    : Fb2TextBase(parent)
    , m_noteView(0)
    , m_thread(0)
{
    setPage(new Fb2TextPage(this));
    page()->setNetworkAccessManager(new Fb2NetworkAccessManager(*this));
    page()->setContentEditable(true);
    connect(page(), SIGNAL(contentsChanged()), this, SLOT(fixContents()));
    connect(page(), SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString,QString,QString)));
    connect(this, SIGNAL(loadFinished(bool)), SLOT(loadFinished()));
}

Fb2TextEdit::~Fb2TextEdit()
{
    FB2DELETE(m_noteView);
}

Fb2TextPage * Fb2TextEdit::page()
{
    return qobject_cast<Fb2TextPage*>(Fb2TextBase::page());
}

Fb2NoteView & Fb2TextEdit::noteView()
{
    if (m_noteView) return *m_noteView;
    m_noteView = new Fb2NoteView(qobject_cast<QWidget*>(parent()), url());
    m_noteView->setPage(new Fb2TextPage(this));
    m_noteView->page()->setNetworkAccessManager(page()->networkAccessManager());
    m_noteView->page()->setContentEditable(false);
    m_noteView->setGeometry(QRect(100, 100, 400, 200));
    return *m_noteView;
}

QWebElement Fb2TextEdit::body()
{
    return doc().findFirst("body");
}

QWebElement Fb2TextEdit::doc()
{
    return page()->mainFrame()->documentElement();
}

void Fb2TextEdit::fixContents()
{
    foreach (QWebElement span, doc().findAll("span.apple-style-span[style]")) {
        span.removeAttribute("style");
    }
}

void Fb2TextEdit::mouseMoveEvent(QMouseEvent *event)
{
    m_point = event->pos();
    QWebView::mouseMoveEvent(event);
}

void Fb2TextEdit::linkHovered(const QString &link, const QString &title, const QString &textContent)
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

void Fb2TextEdit::load(const QString &filename, const QString &xml)
{
    if (m_thread) return;
    m_thread = new Fb2ReadThread(this, filename, xml);
    m_thread->start();
}

bool Fb2TextEdit::save(QIODevice *device, const QString &codec)
{
    Fb2SaveWriter writer(*this, device);
    if (!codec.isEmpty()) writer.setCodec(codec.toLatin1());
    bool ok = Fb2SaveHandler(writer).save();
    if (ok) page()->undoStack()->setClean();
    return ok;
}

bool Fb2TextEdit::save(QByteArray *array)
{
    Fb2SaveWriter writer(*this, array);
    return Fb2SaveHandler(writer).save();
}

bool Fb2TextEdit::save(QString *string)
{
    // Use class QByteArray instead QString
    // to store information about encoding.
    QByteArray data;
    bool ok = save(&data);
    if (ok) *string = QString::fromUtf8(data.data());
    return ok;
}

void Fb2TextEdit::data(QString name, QByteArray data)
{
    m_files.set(name, data);
}

void Fb2TextEdit::html(QString name, QString html)
{
    static int number = 0;
    setHtml(html, QUrl(QString("fb2:/%1/").arg(number++)));
    if (m_thread) m_thread->deleteLater();
    m_thread = 0;
}

void Fb2TextEdit::zoomIn()
{
    qreal zoom = zoomFactor();
    setZoomFactor(zoom * 1.1);
}

void Fb2TextEdit::zoomOut()
{
    qreal zoom = zoomFactor();
    setZoomFactor(zoom * 0.9);
}

void Fb2TextEdit::zoomReset()
{
    setZoomFactor(1);
}

bool Fb2TextEdit::UndoEnabled()
{
    return pageAction(QWebPage::Undo)->isEnabled();
}

bool Fb2TextEdit::RedoEnabled()
{
    return pageAction(QWebPage::Redo)->isEnabled();
}

bool Fb2TextEdit::CutEnabled()
{
    return pageAction(QWebPage::Cut)->isEnabled();
}

bool Fb2TextEdit::CopyEnabled()
{
    return pageAction(QWebPage::Copy)->isEnabled();
}

bool Fb2TextEdit::BoldChecked()
{
    return pageAction(QWebPage::ToggleBold)->isChecked();
}

bool Fb2TextEdit::ItalicChecked()
{
    return pageAction(QWebPage::ToggleItalic)->isChecked();
}

bool Fb2TextEdit::StrikeChecked()
{
    return pageAction(QWebPage::ToggleStrikethrough)->isChecked();
}

bool Fb2TextEdit::SubChecked()
{
    return pageAction(QWebPage::ToggleSubscript)->isChecked();
}

bool Fb2TextEdit::SupChecked()
{
    return pageAction(QWebPage::ToggleSuperscript)->isChecked();
}

void Fb2TextEdit::find()
{
    Fb2TextFindDlg dlg(*this);
    dlg.exec();
}

void Fb2TextEdit::insertImage()
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

void Fb2TextEdit::insertNote()
{
    Fb2NoteDlg dlg(*this);
    dlg.exec();
}


void Fb2TextEdit::insertLink()
{
}

void Fb2TextEdit::execCommand(const QString &cmd, const QString &arg)
{
    QString javascript = QString("document.execCommand(\"%1\",false,\"%2\")").arg(cmd).arg(arg);
    page()->mainFrame()->evaluateJavaScript(javascript);
}

void Fb2TextEdit::loadFinished()
{
    Fb2TextElement element = body().findFirst("div.body");
    if (element.isNull()) element = body();
    element.select();
}

void Fb2TextEdit::insertTitle()
{
    page()->undoStack()->beginMacro("Insert title");
    static const QString javascript = FB2::read(":/js/insert_title.js");
    page()->mainFrame()->evaluateJavaScript(javascript);
    page()->undoStack()->endMacro();
}

//---------------------------------------------------------------------------
//  Fb2TextFrame
//---------------------------------------------------------------------------

Fb2TextFrame::Fb2TextFrame(QWidget* parent)
    : QFrame(parent)
    , view(this)
    , dock(0)
{
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Sunken);

    QLayout * layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(&view);
}

Fb2TextFrame::~Fb2TextFrame()
{
    if (dock) dock->deleteLater();
}

void Fb2TextFrame::showInspector()
{
    if (dock) {
        dock->show();
        return;
    }

    QMainWindow * main = qobject_cast<QMainWindow*>(parent());
    if (!main) return;

    dock = new QDockWidget(tr("Web inspector"), this);
    dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
    main->addDockWidget(Qt::BottomDockWidgetArea, dock);

    QWebInspector * inspector = new QWebInspector(this);
    inspector->setPage(view.page());
    dock->setWidget(inspector);
}

void Fb2TextFrame::hideInspector()
{
    if (dock) dock->hide();
}
