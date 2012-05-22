#include <QtGui>
#include <QtDebug>

#include "fb2read.h"
#include "fb2xml2.h"

//---------------------------------------------------------------------------
//  Fb2ReadThread
//---------------------------------------------------------------------------

Fb2ReadThread::Fb2ReadThread(QObject *parent, const QString &filename, const QString &xml)
    : QThread(parent)
    , m_filename(filename)
    , m_abort(false)
    , m_xml(xml)
{
    connect(this, SIGNAL(html(QString, QString)), parent, SLOT(html(QString, QString)));
}

Fb2ReadThread::~Fb2ReadThread()
{
    stop();
    wait();
}

void Fb2ReadThread::stop()
{
    QMutexLocker locker(&mutex);
    Q_UNUSED(locker);
    m_abort = true;
}

void Fb2ReadThread::run()
{
    if (parse()) emit html(m_filename, m_html);
}

bool Fb2ReadThread::parse()
{
    QXmlStreamWriter writer(&m_html);
    Fb2ReadHandler handler(*this, writer);
    #ifdef _WIN32
        QXmlSimpleReader reader;
    #else
        XML2::XmlReader reader;
    #endif
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    if (m_xml.isEmpty()) {
        QFile file(m_filename);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            qCritical() << QObject::tr("Cannot read file %1: %2.").arg(m_filename).arg(file.errorString());
            return false;
        }
        QXmlInputSource source(&file);
        return reader.parse(source);
    } else {
        QXmlInputSource source;
        source.setData(m_xml);
        return reader.parse(source);
    }
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::RootHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(Fb2ReadHandler::RootHandler)
    FB2_KEY( Style  , "stylesheet"  );
    FB2_KEY( Descr  , "description" );
    FB2_KEY( Body   , "body"        );
    FB2_KEY( Binary , "binary"      );
FB2_END_KEYHASH

Fb2ReadHandler::RootHandler::RootHandler(Fb2ReadHandler &owner, const QString &name)
    : BaseHandler(owner, name)
{
    writer().writeStartDocument();
    writer().writeStartElement("html");
    writer().writeStartElement("body");
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::RootHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    switch (toKeyword(name)) {
        case Body   : return new TextHandler(m_owner, name, atts, "div", name);
        case Descr  : return new DescrHandler(m_owner, name, atts);
        case Binary : return new BinaryHandler(m_owner, name, atts);
        default: return NULL;
    }
}

void Fb2ReadHandler::RootHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    writer().writeEndElement();
    writer().writeEndElement();
    writer().writeEndDocument();
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::HeadHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(Fb2ReadHandler::HeadHandler)
    FB2_KEY( Image , "image" );
FB2_END_KEYHASH

Fb2ReadHandler::HeadHandler::HeadHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts)
    : BaseHandler(owner, name)
    , m_empty(true)
{
    writer().writeStartElement("div");
    writer().writeAttribute("class", name);
    int count = atts.count();
    for (int i = 0; i < count; i++) {
        writer().writeAttribute("fb2:" + atts.qName(i), atts.value(i));
    }
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::HeadHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    Q_UNUSED(atts);
    m_empty = false;
    switch (toKeyword(name)) {
        case Image: return new ImageHandler(m_owner, name, atts);
        default: return new HeadHandler(m_owner, name, atts);
    }
}

void Fb2ReadHandler::HeadHandler::TxtTag(const QString &text)
{
    m_empty = false;
    writer().writeCharacters(text);
}

void Fb2ReadHandler::HeadHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_empty) writer().writeCharacters(" ");
    writer().writeEndElement();
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::DescrHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(Fb2ReadHandler::DescrHandler)
    FB2_KEY( Title    , "title-info"    );
    FB2_KEY( Document , "document-info" );
    FB2_KEY( Publish  , "publish-info"  );
    FB2_KEY( Custom   , "custom-info"   );
FB2_END_KEYHASH

Fb2ReadHandler::DescrHandler::DescrHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts)
    : HeadHandler(owner, name, atts)
{
    writer().writeAttribute("id", m_owner.newId());
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::DescrHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    Q_UNUSED(atts);
    switch (toKeyword(name)) {
        case Title :
            return new TitleHandler(m_owner, name, atts);
        case Document :
        case Publish :
        case Custom :
            return new HeadHandler(m_owner, name, atts);
        default:
            return NULL;
    }
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::TitleHandler
//---------------------------------------------------------------------------

Fb2ReadHandler::TitleHandler::TitleHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts)
    : HeadHandler(owner, name, atts)
{
    writer().writeAttribute("id", m_owner.newId());
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::TitleHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    if (name == "annotation") return new TextHandler(m_owner, name, atts, "div", name);
    return new HeadHandler(m_owner, name, atts);
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::TextHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(Fb2ReadHandler::TextHandler)
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

Fb2ReadHandler::TextHandler::TextHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts, const QString &tag, const QString &style)
    : BaseHandler(owner, name)
    , m_parent(NULL)
    , m_tag(tag)
    , m_style(style)
{
    Init(atts);
}

Fb2ReadHandler::TextHandler::TextHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag, const QString &style)
    : BaseHandler(parent->m_owner, name)
    , m_parent(parent)
    , m_tag(tag)
    , m_style(style)
{
    Init(atts);
}

void Fb2ReadHandler::TextHandler::Init(const QXmlAttributes &atts)
{
    if (m_tag.isEmpty()) return;
    writer().writeStartElement(m_tag);
    QString id = Value(atts, "id");
    if (!id.isEmpty()) {
        if (m_style == "section" && isNotes()) m_style = "note";
        writer().writeAttribute("id", id);
    } else if (m_tag == "div" || m_tag == "img") {
        writer().writeAttribute("id", m_owner.newId());
    }
    if (!m_style.isEmpty()) {
        if (m_style == "body" && Value(atts, "name").toLower() == "notes") m_style = "notes";
        writer().writeAttribute("class", m_style);
    }
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::TextHandler::NewTag(const QString &name, const QXmlAttributes &atts)
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
        default: ;
    }
    return new TextHandler(this, name, atts, tag, style);
}

void Fb2ReadHandler::TextHandler::TxtTag(const QString &text)
{
    writer().writeCharacters(text);
}

void Fb2ReadHandler::TextHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_tag.isEmpty()) return;
    if (m_tag == "div") writer().writeCharacters(" ");
    writer().writeEndElement();
}

bool Fb2ReadHandler::TextHandler::isNotes() const
{
    if (m_style == "notes") return true;
    return m_parent ? m_parent->isNotes() : false;
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::AnchorHandler
//---------------------------------------------------------------------------

Fb2ReadHandler::AnchorHandler::AnchorHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts)
    : TextHandler(parent, name, atts, "a")
{
    QString href = Value(atts, "href");
    writer().writeAttribute("href", href);
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::ImageHandler
//---------------------------------------------------------------------------

Fb2ReadHandler::ImageHandler::ImageHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts)
    : TextHandler(owner, name, atts, "img")
{
    QString href = Value(atts, "href");
    while (href.left(1) == "#") href.remove(0, 1);
    QString path = m_owner.getFile(href);
    writer().writeAttribute("src", path);
    writer().writeAttribute("alt", href);
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::BinaryHandler
//---------------------------------------------------------------------------

Fb2ReadHandler::BinaryHandler::BinaryHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts)
    : BaseHandler(owner, name)
    , m_file(Value(atts, "id"))
{
}

void Fb2ReadHandler::BinaryHandler::TxtTag(const QString &text)
{
    m_text += text;
}

void Fb2ReadHandler::BinaryHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    QByteArray in; in.append(m_text);
    if (!m_file.isEmpty()) m_owner.addFile(m_file, QByteArray::fromBase64(in));
}


//---------------------------------------------------------------------------
//  Fb2ReadHandler
//---------------------------------------------------------------------------

Fb2ReadHandler::Fb2ReadHandler(Fb2ReadThread &thread, QXmlStreamWriter &writer)
    : Fb2XmlHandler()
    , m_thread(thread)
    , m_writer(writer)
    , m_id(0)
{
    m_writer.setAutoFormatting(true);
    m_writer.setAutoFormattingIndent(2);
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::CreateRoot(const QString &name, const QXmlAttributes &atts)
{
    Q_UNUSED(atts);
    if (name == "fictionbook") return new RootHandler(*this, name);
    m_error = QObject::tr("The file is not an FB2 file.");
    return 0;
}

QString Fb2ReadHandler::getFile(const QString &name)
{
    QString path;
    QMetaObject::invokeMethod(m_thread.parent(), "temp", Qt::DirectConnection, Q_RETURN_ARG(QString, path), Q_ARG(QString, name));
    return path;
}

void Fb2ReadHandler::addFile(const QString &name, const QByteArray &data)
{
    QMetaObject::invokeMethod(m_thread.parent(), "data", Qt::QueuedConnection, Q_ARG(QString, name), Q_ARG(QByteArray, data));
}

QString Fb2ReadHandler::newId()
{
    return QString("FB2E%1").arg(++m_id);
}

