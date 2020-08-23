#include <QtGui>
#include <QtDebug>

#include "fb2page.hpp"
#include "fb2save.hpp"
#include "fb2text.hpp"
#include "fb2utils.h"
#include "fb2html.h"

#include <QAbstractNetworkCache>
#include <QBuffer>
#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopedPointer>
#include <QTextCodec>
#include <QWebFrame>
#include <QWebPage>
#include <QtDebug>

#define XMLAutoFormatting
//#define ImgTypePrint

//---------------------------------------------------------------------------
//  FbSaveDialog
//---------------------------------------------------------------------------

FbSaveDialog::FbSaveDialog(QWidget *parent, Qt::WindowFlags f)
    : QFileDialog(parent, f)
{
    init();
}

FbSaveDialog::FbSaveDialog(QWidget *parent, const QString &caption, const QString &directory, const QString &filter)
    : QFileDialog(parent, caption, directory, filter)
{
    init();
}

void FbSaveDialog::init()
{
    QMap<QString, QString> codecMap;
    for (const int &mib: QTextCodec::availableMibs()) {
        QTextCodec *codec = QTextCodec::codecForMib(mib);

        QString sortKey = codec->name().toUpper();
        int rank;

        if (sortKey.startsWith("UTF-8")) {
            rank = 1;
        } else if (sortKey.startsWith("KOI8")) {
            rank = 2;
        } else if (sortKey.startsWith("WINDOWS")) {
            rank = 3;
        } else {
            rank = 4;
        }
        sortKey.prepend(QChar('0' + rank));
        codecMap.insert(sortKey, codec->name());
    }

    setAcceptMode(AcceptSave);
    setConfirmOverwrite(true);
    setDefaultSuffix("fb2");

    QStringList filters;
    filters << tr("Fiction book files (*.fb2)");
    filters << tr("Any files (*.*)");
    setNameFilters(filters);

    combo = new QComboBox(this);
    for (const QString &codec: codecMap) {
        combo->addItem(codec);
    }
    combo->setCurrentIndex(0);

    label = new QLabel(this);
    label->setText(tr("&Encoding"));
    label->setBuddy(combo);

    layout()->addWidget(label);
    layout()->addWidget(combo);
}

QString FbSaveDialog::fileName() const
{
    for (const QString &filename: selectedFiles()) {
        return filename;
    }
    return QString();
}

QString FbSaveDialog::codec() const
{
    return combo->currentText();
}

//---------------------------------------------------------------------------
//  FbHtmlHandler
//---------------------------------------------------------------------------

QString FbHtmlHandler::local(const QString &name)
{
    return name.mid(name.lastIndexOf(":"));
}

void FbHtmlHandler::onAttr(const QString &name, const QString &value)
{
    m_atts.append(name, "", local(name), value);
}

void FbHtmlHandler::onNew(const QString &name)
{
    startElement("", local(name), name, m_atts);
    m_atts.clear();
}

void FbHtmlHandler::onTxt(const QString &text)
{
    m_lastTextLength = text.length();
    characters(text);
}

void FbHtmlHandler::onCom(const QString &text)
{
    comment(text);
}

void FbHtmlHandler::onEnd(const QString &name)
{
    endElement("", local(name), name);
}

//---------------------------------------------------------------------------
//  FbSaveWriter
//---------------------------------------------------------------------------

FbSaveWriter::FbSaveWriter(FbTextEdit &view, QByteArray *array)
    : QXmlStreamWriter(array)
    , m_view(view)
    , m_string(0)
    , m_anchor(0)
    , m_focus(0)
{
    if (QWebFrame * frame = m_view.page()->mainFrame()) {
        m_style = frame->findFirstElement("html>head>style#origin").toPlainText();
    }
#ifdef XMLAutoFormatting
    setAutoFormatting(true);
#endif
}

FbSaveWriter::FbSaveWriter(FbTextEdit &view, QIODevice *device)
    : QXmlStreamWriter(device)
    , m_view(view)
    , m_string(0)
    , m_anchor(0)
    , m_focus(0)
{
#ifdef XMLAutoFormatting
    setAutoFormatting(true);
#endif
}

FbSaveWriter::FbSaveWriter(FbTextEdit &view, QString *string)
    : QXmlStreamWriter(string)
    , m_view(view)
    , m_string(string)
    , m_anchor(0)
    , m_focus(0)
{
#ifdef XMLAutoFormatting
    setAutoFormatting(true);
#endif
}

void FbSaveWriter::writeComment(const QString &ch)
{
    writeLineEnd();
    QXmlStreamWriter::writeComment(ch);

}

void FbSaveWriter::writeStartDocument()
{
    if (device()) {
        QXmlStreamWriter::writeStartDocument();
    } else if (m_string) {
        m_string->append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    }
}

void FbSaveWriter::writeStartElement(const QString &name, int level)
{
#ifndef XMLAutoFormatting
    Q_UNUSED(level)
    if (level) writeLineEnd();
    for (int i = 1; i < level; ++i) writeCharacters("  ");
#endif
    QXmlStreamWriter::writeStartElement(name);
}

void FbSaveWriter::writeEndElement(int level)
{
#ifndef XMLAutoFormatting
    Q_UNUSED(level)
    if (level) writeLineEnd();
    for (int i = 1; i < level; ++i) writeCharacters("  ");
#endif
    QXmlStreamWriter::writeEndElement();
}

void FbSaveWriter::writeLineEnd()
{
#ifndef XMLAutoFormatting
    writeCharacters("\n");
#endif
}

QByteArray FbSaveWriter::downloadFile(const QUrl &url)
{
    QNetworkRequest request(url);
    QNetworkAccessManager * network = m_view.page()->networkAccessManager();
    QScopedPointer<QNetworkReply> reply(network->get(request));
    if (reply.isNull()) return QByteArray();

    QEventLoop loop;
    QObject::connect(reply.data(), SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    return reply->readAll();
}

QString FbSaveWriter::append(const QString &name)
{
    if (m_names.indexOf(name) < 0) {
        m_names.append(name);
    }
    return name;
}

QString FbSaveWriter::filename(const QString &path)
{
    FbStore *store = m_view.store();
    if (!store) return QString();

    if (path.left(1) == "#") {
        QString name = path.mid(1);
        if (store->exists(name)) {
            return append(name);
        } else {
            return QString();
        }
    } else {
        QUrl url = path;
        QByteArray data = downloadFile(url);
        if (data.size() == 0) return QString();
        QString name = store->add(url.path(), data);
        return append(name);
    }
}

void FbSaveWriter::writeStyle()
{
    if (m_style.isEmpty()) return;

    const QString postfix = "\n  ";
    writeStartElement("stylesheet", 2);
    writeAttribute("type", "text/css");
    writeCharacters(postfix);

    QStringList list = m_style.split("}", QString::SkipEmptyParts);
    QString line;
    for (const QString &str: list) {
        line = str.simplified();
        if (line.isEmpty()) continue;
        writeCharacters("  " + line + "}" + postfix);
    }

    QXmlStreamWriter::writeEndElement();
}

void FbSaveWriter::writeFiles()
{
    FbStore *store = m_view.store();
    if (!store) return;

    QStringListIterator it(m_names);
    while (it.hasNext()) {
        QString name = it.next();
        if (name.isEmpty()) continue;
        FbBinary * file = store->get(name);
        if (!file) continue;
        writeStartElement("binary", 2);
        writeAttribute("id", name);
        QByteArray array = file->data();
        QString data = array.toBase64();
        writeContentType(name, array);
        writeLineEnd();
        int pos = 0;
        while (true) {
            QString text = data.mid(pos, 76);
            if (text.isEmpty()) break;
            writeCharacters(text);
            writeLineEnd();
            pos += 76;
        }
        writeCharacters("  ");
        QXmlStreamWriter::writeEndElement();
    }
}

void FbSaveWriter::writeContentType(const QString &name, QByteArray &data)
{
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);
    QString type = QImageReader::imageFormat(&buffer);
#ifdef ImgTypePrint
    qCritical()<<"Img type: "<< type;
#endif
    if (type.isEmpty()) {
        qCritical() << QObject::tr("Unknown image format: %1").arg(name);
        return;
    }
    type.prepend("image/");
    writeAttribute("content-type", type);
}

void FbSaveWriter::setAnchor(int offset)
{
    if (m_string) m_anchor = m_string->length() + offset;
}

void FbSaveWriter::setFocus(int offset)
{
    if (m_string) m_focus = m_string->length() + offset;
}

//---------------------------------------------------------------------------
//  FbSaveHandler::TextHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(FbSaveHandler::TextHandler)
    FB2_KEY( Origin  , "table"  );
    FB2_KEY( Origin  , "td"     );
    FB2_KEY( Origin  , "th"     );
    FB2_KEY( Origin  , "tr"     );
    FB2_KEY( Origin  , "a"      );
    FB2_KEY( Image   , "img"    );
    FB2_KEY( Parag   , "p"      );
    FB2_KEY( Strong  , "b"      );
    FB2_KEY( Emphas  , "i"      );
    FB2_KEY( Span    , "span"   );
    FB2_KEY( Strike  , "strike" );
    FB2_KEY( Sub     , "sub"    );
    FB2_KEY( Sup     , "sup"    );
    FB2_KEY( Code    , "tt"     );
FB2_END_KEYHASH

FbSaveHandler::TextHandler::TextHandler(FbSaveWriter &writer, const QString &name, const QXmlAttributes &atts, const QString &tag)
    : NodeHandler(name)
    , m_writer(writer)
    , m_tag(tag)
    , m_level(1)
    , m_hasChild(false)
{
    if (tag.isEmpty()) return;
    m_writer.writeStartElement(m_tag, m_level);
    writeAtts(atts);
}

FbSaveHandler::TextHandler::TextHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag)
    : NodeHandler(name)
    , m_writer(parent->m_writer)
    , m_tag(tag)
    , m_level(parent->nextLevel())
    , m_hasChild(false)
{
    if (tag.isEmpty()) return;
    m_writer.writeStartElement(m_tag, m_level);
    writeAtts(atts);
}

void FbSaveHandler::TextHandler::writeAtts(const QXmlAttributes &atts)
{
    int count = atts.count();
    for (int i = 0; i < count; ++i) {
        QString name = atts.qName(i);
        QString value = atts.value(i);
        if (m_tag == "image") {
            if (name == "src") {
                name = "l:href";
                value = m_writer.filename(value).prepend('#');
            }
        } else if (m_tag == "a") {
            if (name == "href") name = "l:href";
        }
        m_writer.writeAttribute(name, value);
    }
}

FbXmlHandler::NodeHandler * FbSaveHandler::TextHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    m_hasChild = true;
    QString tag = QString();
    switch (toKeyword(name)) {
        case Origin    : tag = name; break;
        case Parag     : return new ParagHandler(this, name, atts);
        case Span      : return new SpanHandler(this, name, atts);
        case Image     : tag = "image"         ; break;
        case Strong    : tag = "strong"        ; break;
        case Emphas    : tag = "emphasis"      ; break;
        case Strike    : tag = "strikethrough" ; break;
        case Code      : tag = "code"          ; break;
        case Sub       : tag = "sub"           ; break;
        case Sup       : tag = "sup"           ; break;
        default: if (name.left(3) == "fb:") tag = name.mid(3);
    }
    return new TextHandler(this, name, atts, tag);
}

void FbSaveHandler::TextHandler::TxtTag(const QString &text)
{
    m_writer.writeCharacters(text);
}

void FbSaveHandler::TextHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_tag.isEmpty()) return;
    m_writer.writeEndElement(m_hasChild ? m_level : 0);
}

int FbSaveHandler::TextHandler::nextLevel() const
{
    return m_level ? m_level + 1 : 0;
}

//---------------------------------------------------------------------------
//  FbSaveHandler::RootHandler
//---------------------------------------------------------------------------

FbSaveHandler::RootHandler::RootHandler(FbSaveWriter &writer, const QString &name)
    : NodeHandler(name)
    , m_writer(writer)
{
}

FbXmlHandler::NodeHandler * FbSaveHandler::RootHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    Q_UNUSED(atts);
    return name == "body" ? new BodyHandler(m_writer, name) : NULL;
}

//---------------------------------------------------------------------------
//  FbSaveHandler::BodyHandler
//---------------------------------------------------------------------------

FbSaveHandler::BodyHandler::BodyHandler(FbSaveWriter &writer, const QString &name)
    : TextHandler(writer, name, QXmlAttributes(), "FictionBook")
{
    m_writer.writeAttribute("xmlns", "http://www.gribuser.ru/xml/fictionbook/2.0");
    m_writer.writeAttribute("xmlns:l", "http://www.w3.org/1999/xlink");
    m_writer.writeStyle();
}

void FbSaveHandler::BodyHandler::EndTag(const QString &name)
{
    m_writer.writeFiles();
    TextHandler::EndTag(name);
}

//---------------------------------------------------------------------------
//  FbSaveHandler::SpanHandler
//---------------------------------------------------------------------------

FbSaveHandler::SpanHandler::SpanHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts)
    : TextHandler(parent, name, atts, Value(atts, "class") == "Apple-style-span" ? "" : "style")
{
}

//---------------------------------------------------------------------------
//  FbSaveHandler::ParagHandler
//---------------------------------------------------------------------------

FbSaveHandler::ParagHandler::ParagHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts)
    : TextHandler(parent, name, atts, "")
    , m_parent(parent->tag())
    , m_empty(true)
{
    int count = atts.count();
    for (int i = 0; i < count; ++i) {
        QString qName = atts.qName(i);
        QString value = atts.value(i);
        if (qName == "fb:class") {
            m_class = value;
        } else {
            m_atts.append(qName, "", "", value);
        }
    }
}

FbXmlHandler::NodeHandler * FbSaveHandler::ParagHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    if (m_empty && name != "br") start();
    return TextHandler::NewTag(name, atts);
}

void FbSaveHandler::ParagHandler::TxtTag(const QString &text)
{
    if (m_empty) {
        if (isWhiteSpace(text)) return;
        start();
    }
    TextHandler::TxtTag(text);
}

void FbSaveHandler::ParagHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_empty) m_writer.writeStartElement("empty-line", m_level);
    m_writer.writeEndElement(0);
}

void FbSaveHandler::ParagHandler::start()
{
    if (!m_empty) return;
    QString tag = "p";
    if (m_class.isEmpty()) {
        if (m_parent == "stanza") tag = "v";
    } else {
        tag = m_class;
    }
    m_writer.writeStartElement(tag, m_level);
    writeAtts(m_atts);
    m_empty = false;
}

//---------------------------------------------------------------------------
//  FbSaveHandler
//---------------------------------------------------------------------------

FbSaveHandler::FbSaveHandler(FbSaveWriter &writer)
    : FbHtmlHandler()
    , m_writer(writer)
{
}

bool FbSaveHandler::comment(const QString& ch)
{
    m_writer.writeComment(ch);
    return true;
}

void FbSaveHandler::onAnchor(int offset)
{
    m_writer.setAnchor(offset - m_lastTextLength);
}

void FbSaveHandler::onFocus(int offset)
{
    m_writer.setFocus(offset - m_lastTextLength);
}

FbXmlHandler::NodeHandler * FbSaveHandler::CreateRoot(const QString &name, const QXmlAttributes &atts)
{
    Q_UNUSED(atts);
    if (name == "html") return new RootHandler(m_writer, name);
    m_error = QObject::tr("The tag <html> was not found.");
    return 0;
}

void FbSaveHandler::setDocumentInfo(QWebFrame * frame)
{
    QString info1 = qApp->applicationName() += QString(" ") += qApp->applicationVersion();
    QDateTime now = QDateTime::currentDateTime();
    QString info2 = now.toString("dd MMM yyyy");
    QString value = now.toString("yyyy-MM-dd hh:mm:ss");

    FbTextElement parent = frame->documentElement().findFirst("BODY");
    parent = parent["FB:DESCRIPTION"];
    parent = parent["FB:DOCUMENT-INFO"];

    FbTextElement child1 = parent["FB:PROGRAM-USED"];
    child1.setInnerXml(info1);

    FbTextElement child2 = parent["FB:DATE"];
    child2.setInnerXml(info2);
    child2.setAttribute("value", value);
}

bool FbSaveHandler::save()
{
    FbTextPage *page = m_writer.view().page();
    if (!page) return false;

    QWebFrame *frame = page->mainFrame();
    if (!frame) return false;

    m_writer.writeStartDocument();
    if (page->isModified()) setDocumentInfo(frame);
    QString javascript = jScript("export.js");
    frame->addToJavaScriptWindowObject("handler", this);
    frame->evaluateJavaScript(javascript);
    m_writer.writeEndDocument();

    return true;
}
