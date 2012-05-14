#include <QtGui>
#include <QtDebug>

#include "fb2read.h"

//---------------------------------------------------------------------------
//  Fb2ReadThread
//---------------------------------------------------------------------------

Fb2ReadThread::Fb2ReadThread(QObject *parent, const QString &filename)
    : QThread(parent)
    , m_filename(filename)
    , m_abort(false)
{
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

void Fb2ReadThread::onFile(const QString &name, const QString &path)
{
    emit file(name, path);
}

bool Fb2ReadThread::parse()
{
    QFile file(m_filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCritical() << QObject::tr("Cannot read file %1: %2.").arg(m_filename).arg(file.errorString());
        return false;
    }
    Fb2ReadHandler handler(*this);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    QXmlInputSource source(&file);
    return reader.parse(source);
}

//---------------------------------------------------------------------------
//  Fb2ReadWriter
//---------------------------------------------------------------------------

Fb2ReadWriter::Fb2ReadWriter(Fb2ReadThread &thread)
    : QXmlStreamWriter(thread.data())
    , m_thread(thread)
    , m_id(0)
{
    setAutoFormatting(true);
    setAutoFormattingIndent(2);
}

QString Fb2ReadWriter::addFile(const QString &name, const QByteArray &data)
{
    QString path = getFile(name);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        m_thread.onFile(name, path);
    }
    return path;
}

QString Fb2ReadWriter::getFile(const QString &name)
{
    StringHash::const_iterator i = m_hash.find(name);
    if (i == m_hash.end()) {
        QTemporaryFile file;
        file.setAutoRemove(false);
        file.open();
        return m_hash.insert(name, file.fileName()).value();
    } else {
        return i.value();
    }
}

QString Fb2ReadWriter::newId()
{
    return QString("FB2E%1").arg(++m_id);
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

Fb2ReadHandler::RootHandler::RootHandler(Fb2ReadWriter &writer, const QString &name)
    : BaseHandler(writer, name)
{
    m_writer.writeStartElement("html");
    m_writer.writeStartElement("body");
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::RootHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    switch (toKeyword(name)) {
        case Body   : return new TextHandler(m_writer, name, attributes, "div", name);
        case Descr  : return new DescrHandler(m_writer, name);
        case Binary : return new BinaryHandler(m_writer, name, attributes);
        default: return NULL;
    }
}

void Fb2ReadHandler::RootHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    m_writer.writeEndElement();
    m_writer.writeEndElement();
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::HeadHandler
//---------------------------------------------------------------------------

Fb2ReadHandler::HeadHandler::HeadHandler(Fb2ReadWriter &writer, const QString &name, bool hide)
    : BaseHandler(writer, name)
{
    m_writer.writeStartElement("div");
    m_writer.writeAttribute("class", name);
    if (hide) m_writer.writeAttribute("style", "display:none");
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::HeadHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(attributes);
    return new HeadHandler(m_writer, name);
}

void Fb2ReadHandler::HeadHandler::TxtTag(const QString &text)
{
    m_writer.writeCharacters(text);
}

void Fb2ReadHandler::HeadHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    m_writer.writeCharacters(" ");
    m_writer.writeEndElement();
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

Fb2ReadHandler::DescrHandler::DescrHandler(Fb2ReadWriter &writer, const QString &name)
    : HeadHandler(writer, name)
{
    m_writer.writeAttribute("id", m_writer.newId());
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::DescrHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(attributes);
    switch (toKeyword(name)) {
        case Title :
            return new TitleHandler(m_writer, name);
        case Document :
        case Publish :
        case Custom :
            return new HeadHandler(m_writer, name, true);
        default:
            return NULL;
    }
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::TitleHandler
//---------------------------------------------------------------------------

Fb2ReadHandler::TitleHandler::TitleHandler(Fb2ReadWriter &writer, const QString &name)
    : HeadHandler(writer, name)
{
    m_writer.writeAttribute("id", m_writer.newId());
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::TitleHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    if (name == "annotation") return new TextHandler(m_writer, name, attributes, "div", name);
    return new HeadHandler(m_writer, name, true);
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

Fb2ReadHandler::TextHandler::TextHandler(Fb2ReadWriter &writer, const QString &name, const QXmlAttributes &attributes, const QString &tag, const QString &style)
    : BaseHandler(writer, name)
    , m_parent(NULL)
    , m_tag(tag)
    , m_style(style)
{
    Init(attributes);
}

Fb2ReadHandler::TextHandler::TextHandler(TextHandler *parent, const QString &name, const QXmlAttributes &attributes, const QString &tag, const QString &style)
    : BaseHandler(parent->m_writer, name)
    , m_parent(parent)
    , m_tag(tag)
    , m_style(style)
{
    Init(attributes);
}

void Fb2ReadHandler::TextHandler::Init(const QXmlAttributes &attributes)
{
    if (m_tag.isEmpty()) return;
    m_writer.writeStartElement(m_tag);
    QString id = Value(attributes, "id");
    if (!id.isEmpty()) {
        if (m_style == "section" && isNotes()) m_style = "note";
        m_writer.writeAttribute("id", id);
    } else if (m_tag == "div" || m_tag == "img") {
        m_writer.writeAttribute("id", m_writer.newId());
    }
    if (!m_style.isEmpty()) {
        if (m_style == "body" && Value(attributes, "name").toLower() == "notes") m_style = "notes";
        m_writer.writeAttribute("class", m_style);
    }
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::TextHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    QString tag, style;
    switch (toKeyword(name)) {
        case Anchor    : return new AnchorHandler(this, name, attributes);
        case Image     : return new ImageHandler(this, name, attributes);
        case Section   : tag = "div"; style = name; break;
        case Parag     : tag = "p";   break;
        case Strong    : tag = "b";   break;
        case Emphas    : tag = "i";   break;
        case Strike    : tag = "s";   break;
        case Code      : tag = "tt";  break;
        case Sub       : tag = "sub"; break;
        case Sup       : tag = "sup"; break;
    }
    return new TextHandler(this, name, attributes, tag, style);
}

void Fb2ReadHandler::TextHandler::TxtTag(const QString &text)
{
    m_writer.writeCharacters(text);
}

void Fb2ReadHandler::TextHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_tag.isEmpty()) return;
    if (m_tag == "div") m_writer.writeCharacters(" ");
    m_writer.writeEndElement();
}

bool Fb2ReadHandler::TextHandler::isNotes() const
{
    if (m_style == "notes") return true;
    return m_parent ? m_parent->isNotes() : false;
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::AnchorHandler
//---------------------------------------------------------------------------

Fb2ReadHandler::AnchorHandler::AnchorHandler(TextHandler *parent, const QString &name, const QXmlAttributes &attributes)
    : TextHandler(parent, name, attributes, "a")
{
    QString href = Value(attributes, "href");
    m_writer.writeAttribute("href", href);
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::ImageHandler
//---------------------------------------------------------------------------

Fb2ReadHandler::ImageHandler::ImageHandler(TextHandler *parent, const QString &name, const QXmlAttributes &attributes)
    : TextHandler(parent, name, attributes, "img")
{
    QString href = Value(attributes, "href");
    while (href.left(1) == "#") href.remove(0, 1);
    QString path = m_writer.getFile(href);
    m_writer.writeAttribute("src", path);
    m_writer.writeAttribute("alt", href);
}

//---------------------------------------------------------------------------
//  Fb2ReadHandler::BinaryHandler
//---------------------------------------------------------------------------

Fb2ReadHandler::BinaryHandler::BinaryHandler(Fb2ReadWriter &writer, const QString &name, const QXmlAttributes &attributes)
    : BaseHandler(writer, name)
    , m_file(Value(attributes, "id"))
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
    if (!m_file.isEmpty()) m_writer.addFile(m_file, QByteArray::fromBase64(in));
}


//---------------------------------------------------------------------------
//  Fb2ReadHandler
//---------------------------------------------------------------------------

Fb2ReadHandler::Fb2ReadHandler(Fb2ReadThread &thread)
    : Fb2XmlHandler()
    , m_writer(thread)
{
}

Fb2XmlHandler::NodeHandler * Fb2ReadHandler::CreateRoot(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(attributes);
    if (name == "fictionbook") return new RootHandler(m_writer, name);
    m_error = QObject::tr("The file is not an FB2 file.");
    return 0;
}
