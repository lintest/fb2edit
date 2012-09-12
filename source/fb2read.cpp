#include <QtGui>
#include <QtDebug>

#include "fb2read.hpp"
#include "fb2xml2.h"

//---------------------------------------------------------------------------
//  FbReadThread
//---------------------------------------------------------------------------

FbReadThread::FbReadThread(QObject *parent, const QString &filename, const QString &xml)
    : QThread(parent)
    , m_temp(0)
    , m_filename(filename)
    , m_xml(xml)
    , m_abort(false)
{
    connect(this, SIGNAL(html(QString)), parent, SLOT(html(QString)));
}

FbReadThread::~FbReadThread()
{
    stop();
    wait();
}

void FbReadThread::stop()
{
    QMutexLocker locker(&mutex);
    Q_UNUSED(locker);
    m_abort = true;
}

void FbReadThread::run()
{
    if (parse()) emit html(m_html);
}

#ifdef FB2_USE_LIBXML2

bool FbReadThread::parse()
{
    QXmlStreamWriter writer(&m_html);
    FbReadHandler handler(*this, writer);
    XML2::XmlReader reader;
    reader.setContentHandler(&handler);
    reader.setLexicalHandler(&handler);
    reader.setErrorHandler(&handler);
    if (m_xml.isEmpty()) {
        QFile file(m_filename);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            qCritical() << QObject::tr("Cannot read file %1: %2.").arg(m_filename).arg(file.errorString());
            return false;
        }
        return reader.parse(file);
    } else {
        QXmlInputSource source;
        source.setData(m_xml);
        return reader.parse(source);
    }
}

#else

bool FbReadThread::parse()
{
    QXmlStreamWriter writer(&m_html);
    FbReadHandler handler(*this, writer);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setLexicalHandler(&handler);
    reader.setErrorHandler(&handler);
    QXmlInputSource source;
    if (m_xml.isEmpty()) {
        QFile file(m_filename);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            qCritical() << QObject::tr("Cannot read file %1: %2.").arg(m_filename).arg(file.errorString());
            return false;
        }
        source.setData(file.readAll());
    } else {
        source.setData(m_xml);
    }
    return reader.parse(source);
}

#endif

//---------------------------------------------------------------------------
//  FbReadHandler::BaseHandler
//---------------------------------------------------------------------------

void FbReadHandler::BaseHandler::writeAttributes(const QXmlAttributes &atts)
{
    int count = atts.count();
    for (int i = 0; i < count; i++) {
        if (atts.localName(i) == "href") continue;
        QString name = atts.qName(i);
        if (name != "id") name.prepend("fb2_");
        writer().writeAttribute(name, atts.value(i));
    }
}

//---------------------------------------------------------------------------
//  FbReadHandler::RootHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(FbReadHandler::RootHandler)
    FB2_KEY( Style  , "stylesheet"  );
    FB2_KEY( Descr  , "description" );
    FB2_KEY( Body   , "body"        );
    FB2_KEY( Binary , "binary"      );
FB2_END_KEYHASH

FbReadHandler::RootHandler::RootHandler(FbReadHandler &owner, const QString &name)
    : BaseHandler(owner, name)
{
    writer().writeStartElement("body");
}

FbXmlHandler::NodeHandler * FbReadHandler::RootHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    switch (toKeyword(name)) {
        case Body   : return new TextHandler(m_owner, name, atts, "div", name);
        case Descr  : return new DescrHandler(m_owner, name, atts);
        case Style  : return new StyleHandler(m_owner, name, atts);
        case Binary : return new BinaryHandler(m_owner, name, atts);
        default: return NULL;
    }
}

void FbReadHandler::RootHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    writer().writeEndElement();
}

//---------------------------------------------------------------------------
//  FbReadHandler::StyleHandler
//---------------------------------------------------------------------------

FbReadHandler::StyleHandler::StyleHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts)
    : BaseHandler(owner, name)
    , m_empty(true)
{
    writer().writeStartElement("div");
    writer().writeAttribute("class", name);
    writeAttributes(atts);
}

void FbReadHandler::StyleHandler::TxtTag(const QString &text)
{
    writer().writeCharacters(text);
    m_empty = false;
}

void FbReadHandler::StyleHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_empty) writer().writeCharacters(" ");
    writer().writeEndElement();
}

//---------------------------------------------------------------------------
//  FbReadHandler::HeadHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(FbReadHandler::HeadHandler)
    FB2_KEY( Image , "image" );
FB2_END_KEYHASH

FbReadHandler::HeadHandler::HeadHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts)
    : BaseHandler(owner, name)
    , m_empty(true)
{
    writer().writeStartElement("div");
    writer().writeAttribute("class", name);
    writeAttributes(atts);
}

FbXmlHandler::NodeHandler * FbReadHandler::HeadHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    Q_UNUSED(atts);
    m_empty = false;
    switch (toKeyword(name)) {
        case Image: return new ImageHandler(m_owner, name, atts);
        default: return new HeadHandler(m_owner, name, atts);
    }
}

void FbReadHandler::HeadHandler::TxtTag(const QString &text)
{
    m_empty = false;
    writer().writeCharacters(text);
}

void FbReadHandler::HeadHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_empty) writer().writeCharacters(" ");
    writer().writeEndElement();
}

//---------------------------------------------------------------------------
//  FbReadHandler::DescrHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(FbReadHandler::DescrHandler)
    FB2_KEY( Title    , "title-info"    );
    FB2_KEY( Document , "document-info" );
    FB2_KEY( Publish  , "publish-info"  );
    FB2_KEY( Custom   , "custom-info"   );
FB2_END_KEYHASH

FbXmlHandler::NodeHandler * FbReadHandler::DescrHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    Q_UNUSED(atts);
    switch (toKeyword(name)) {
        case Title :
        case Document :
        case Publish :
            return new TitleHandler(m_owner, name, atts);
        case Custom :
            return new HeadHandler(m_owner, name, atts);
        default:
            return NULL;
    }
}

//---------------------------------------------------------------------------
//  FbReadHandler::TitleHandler
//---------------------------------------------------------------------------

FbXmlHandler::NodeHandler * FbReadHandler::TitleHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    if (name == "annotation" || name == "history") {
        return new TextHandler(m_owner, name, atts, "div", name);
    }
    return new HeadHandler(m_owner, name, atts);
}

//---------------------------------------------------------------------------
//  FbReadHandler::TextHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(FbReadHandler::TextHandler)
    FB2_KEY( Section , "annotation"    );
    FB2_KEY( Section , "author"        );
    FB2_KEY( Section , "cite"          );
    FB2_KEY( Section , "date"          );
    FB2_KEY( Section , "epigraph"      );
    FB2_KEY( Section , "poem"          );
    FB2_KEY( Section , "section"       );
    FB2_KEY( Section , "stanza"        );
    FB2_KEY( Section , "subtitle"      );
    FB2_KEY( Section , "title"         );

    FB2_KEY( Anchor  , "a"             );
    FB2_KEY( Table   , "table"         );
    FB2_KEY( Image   , "image"         );

    FB2_KEY( Parag   , "empty-line"    );
    FB2_KEY( Parag   , "p"             );
    FB2_KEY( Parag   , "v"             );

    FB2_KEY( Style   , "style"         );
    FB2_KEY( Strong  , "strong"        );
    FB2_KEY( Emphas  , "emphasis"      );
    FB2_KEY( Strike  , "strikethrough" );
    FB2_KEY( Sub     , "sub"           );
    FB2_KEY( Sup     , "sup"           );
    FB2_KEY( Code    , "code"          );
FB2_END_KEYHASH

FbReadHandler::TextHandler::TextHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts, const QString &tag, const QString &style)
    : BaseHandler(owner, name)
    , m_parent(NULL)
    , m_tag(tag)
    , m_style(style)
{
    Init(atts);
}

FbReadHandler::TextHandler::TextHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag, const QString &style)
    : BaseHandler(parent->m_owner, name)
    , m_parent(parent)
    , m_tag(tag)
    , m_style(style)
{
    Init(atts);
    if (name == "empty-line") {
        writer().writeEmptyElement("br");
    }
}

void FbReadHandler::TextHandler::Init(const QXmlAttributes &atts)
{
    if (m_tag.isEmpty()) return;
    writer().writeStartElement(m_tag);
    QString id = Value(atts, "id");
    if (!m_style.isEmpty()) {
        writer().writeAttribute("class", m_style);
    }
    writeAttributes(atts);
}

FbXmlHandler::NodeHandler * FbReadHandler::TextHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    QString tag, style;
    switch (toKeyword(name)) {
        case Anchor    : return new AnchorHandler(this, name, atts);
        case Image     : return new ImageHandler(m_owner, name, atts);
        case Section   : tag = "div"; style = name; break;
        case Parag     : tag = "p";   break;
        case Strong    : tag = "b";   break;
        case Emphas    : tag = "i";   break;
        case Strike    : tag = "s";   break;
        case Code      : tag = "tt";  break;
        case Sub       : tag = "sub"; break;
        case Sup       : tag = "sup"; break;
        case Style     : tag = "span"; break;
        default: ;
    }
    return new TextHandler(this, name, atts, tag, style);
}

void FbReadHandler::TextHandler::TxtTag(const QString &text)
{
    writer().writeCharacters(text);
}

void FbReadHandler::TextHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_tag.isEmpty()) return;
    if (m_tag == "div") writer().writeCharacters(" ");
    writer().writeEndElement();
}

bool FbReadHandler::TextHandler::isNotes() const
{
    if (m_style == "notes") return true;
    return m_parent ? m_parent->isNotes() : false;
}

//---------------------------------------------------------------------------
//  FbReadHandler::SpanHandler
//---------------------------------------------------------------------------

FbReadHandler::SpanHandler::SpanHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts)
    : TextHandler(parent, name, atts, "span")
{
}

//---------------------------------------------------------------------------
//  FbReadHandler::AnchorHandler
//---------------------------------------------------------------------------

FbReadHandler::AnchorHandler::AnchorHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts)
    : TextHandler(parent, name, atts, "a")
{
    QString href = Value(atts, "href");
    writer().writeAttribute("href", href);
}

//---------------------------------------------------------------------------
//  FbReadHandler::ImageHandler
//---------------------------------------------------------------------------

FbReadHandler::ImageHandler::ImageHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts)
    : TextHandler(owner, name, atts, "img")
{
    QString href = Value(atts, "href");
    writer().writeAttribute("src", href);
}

//---------------------------------------------------------------------------
//  FbReadHandler::BinaryHandler
//---------------------------------------------------------------------------

FbReadHandler::BinaryHandler::BinaryHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts)
    : BaseHandler(owner, name)
    , m_file(Value(atts, "id"))
{
}

void FbReadHandler::BinaryHandler::TxtTag(const QString &text)
{
    m_text += text;
}

void FbReadHandler::BinaryHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    QByteArray in; in.append(m_text);
    if (!m_file.isEmpty()) m_owner.addFile(m_file, QByteArray::fromBase64(in));
}

//---------------------------------------------------------------------------
//  FbReadHandler
//---------------------------------------------------------------------------

FbReadHandler::FbReadHandler(FbReadThread &thread, QXmlStreamWriter &writer)
    : FbXmlHandler()
    , m_thread(thread)
    , m_writer(writer)
    , m_temp(thread.temp())
{
    m_writer.setAutoFormatting(true);
    m_writer.setAutoFormattingIndent(2);

    m_writer.writeStartElement("html");
    m_writer.writeStartElement("head");
    m_writer.writeStartElement("script");
    m_writer.writeAttribute("type", "text/javascript");
    m_writer.writeAttribute("src", "qrc:/js/jquery.js");
    m_writer.writeCharacters(" ");
    m_writer.writeEndElement();
    m_writer.writeEndElement();
}

FbReadHandler::~FbReadHandler()
{
    m_writer.writeEndElement();
}

FbXmlHandler::NodeHandler * FbReadHandler::CreateRoot(const QString &name, const QXmlAttributes &atts)
{
    Q_UNUSED(atts);
    if (name == "fictionbook") return new RootHandler(*this, name);
    m_error = QObject::tr("The file is not an FB2 file.");
    return 0;
}

bool FbReadHandler::comment(const QString& ch)
{
    m_writer.writeComment(ch);
    return true;
}

void FbReadHandler::addFile(const QString &name, const QByteArray &data)
{
    QMetaObject::invokeMethod(m_temp, "data", Qt::QueuedConnection, Q_ARG(QString, name), Q_ARG(QByteArray, data));
}
