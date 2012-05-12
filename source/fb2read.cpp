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
    Fb2Handler handler(*this);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    QXmlInputSource source(&file);
    return reader.parse(source);
}

//---------------------------------------------------------------------------
//  Fb2HtmlWriter
//---------------------------------------------------------------------------

Fb2HtmlWriter::Fb2HtmlWriter(Fb2ReadThread &thread)
    : QXmlStreamWriter(thread.data())
    , m_thread(thread)
    , m_id(0)
{
}

QString Fb2HtmlWriter::addFile(const QString &name, const QByteArray &data)
{
    QString path = getFile(name);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        m_thread.onFile(name, path);
    }
    return path;
}

QString Fb2HtmlWriter::getFile(const QString &name)
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

QString Fb2HtmlWriter::newId()
{
    return QString("FB2E%1").arg(++m_id);
}

//---------------------------------------------------------------------------
//  Fb2Handler::BaseHandler
//---------------------------------------------------------------------------

#define FB2_BEGIN_KEYHASH(x) \
Fb2Handler::x::Keyword Fb2Handler::x::toKeyword(const QString &name) \
{                                                                    \
    static const KeywordHash map;                                    \
    KeywordHash::const_iterator i = map.find(name);                  \
    return i == map.end() ? None : i.value();                        \
}                                                                    \
Fb2Handler::x::KeywordHash::KeywordHash() {

#define FB2_END_KEYHASH }

#define FB2_KEY(key,str) insert(str,key);

static QString Value(const QXmlAttributes &attributes, const QString &name)
{
    int count = attributes.count();
    for (int i = 0; i < count; i++ ) {
        if (attributes.localName(i).compare(name, Qt::CaseInsensitive) == 0) {
            return attributes.value(i);
        }
    }
    return QString();
}

Fb2Handler::BaseHandler::~BaseHandler()
{
    if (m_handler) delete m_handler;
}

bool Fb2Handler::BaseHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);
    m_handler = NewTag(name, attributes); if (m_handler) return true;
//    qCritical() << QObject::tr("Unknown XML child tag: <%1> <%2>").arg(m_name).arg(name);
    m_handler = new BaseHandler(m_writer, name);
    return true;
}

bool Fb2Handler::BaseHandler::doText(const QString &text)
{
    if (m_handler) m_handler->doText(text); else TxtTag(text);
    return true;
}

bool Fb2Handler::BaseHandler::doEnd(const QString &name, bool & exists)
{
    if (m_handler) {
        bool found = exists || name == m_name;
        m_handler->doEnd(name, found);
        if (m_handler->m_closed) { delete m_handler; m_handler = NULL; }
        if (found) { exists = true; return true; }
    }
    bool found = name == m_name;
    if (!found) qCritical() << QObject::tr("Conglict XML tags: <%1> - </%2>").arg(m_name).arg(name);
    m_closed = found || exists;
    if (m_closed) EndTag(m_name);
    exists = found;
    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::RootHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(RootHandler)
    insert("stylesheet", Style);
    insert("description", Descr);
    insert("body", Body);
    insert("binary", Binary);
FB2_END_KEYHASH

Fb2Handler::RootHandler::RootHandler(Fb2HtmlWriter &writer, const QString &name)
    : BaseHandler(writer, name)
{
    m_writer.writeStartElement("html");
    m_writer.writeStartElement("body");
}

Fb2Handler::BaseHandler * Fb2Handler::RootHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    switch (toKeyword(name)) {
        case Body   : return new BodyHandler(m_writer, name, attributes, "div", name);
        case Descr  : return new DescrHandler(m_writer, name);
        case Binary : return new BinaryHandler(m_writer, name, attributes);
        default: return NULL;
    }
}

void Fb2Handler::RootHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    m_writer.writeEndElement();
    m_writer.writeEndElement();
}

//---------------------------------------------------------------------------
//  Fb2Handler::HeadHandler
//---------------------------------------------------------------------------

Fb2Handler::HeadHandler::HeadHandler(Fb2HtmlWriter &writer, const QString &name)
    : BaseHandler(writer, name)
{
    m_writer.writeStartElement("div");
    m_writer.writeAttribute("class", name);
}

Fb2Handler::BaseHandler * Fb2Handler::HeadHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(attributes);
    return new HeadHandler(m_writer, name);
}

void Fb2Handler::HeadHandler::TxtTag(const QString &text)
{
    m_writer.writeCharacters(text);
}

void Fb2Handler::HeadHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    m_writer.writeCharacters(" ");
    m_writer.writeEndElement();
}

//---------------------------------------------------------------------------
//  Fb2Handler::DescrHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(DescrHandler)
    insert( "title-info"    , Title    );
    insert( "document-info" , Document );
    insert( "publish-info"  , Publish  );
    insert( "custom-info"   , Custom   );
FB2_END_KEYHASH

Fb2Handler::BaseHandler * Fb2Handler::DescrHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(attributes);
    switch (toKeyword(name)) {
        case Title :
            return new TitleHandler(m_writer, name);
        case Document :
        case Publish :
        case Custom :
            return new HeadHandler(m_writer, name);
        default:
            return NULL;
    }
}

//---------------------------------------------------------------------------
//  Fb2Handler::TitleHandler
//---------------------------------------------------------------------------

Fb2Handler::BaseHandler * Fb2Handler::TitleHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    if (name == "annotation") return new BodyHandler(m_writer, name, attributes, "div", name);
    return new HeadHandler(m_writer, name);
}

//---------------------------------------------------------------------------
//  Fb2Handler::BodyHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(BodyHandler)
    FB2_KEY( Section, "annotation" );
    FB2_KEY( Section, "author"     );
    FB2_KEY( Section, "cite"       );
    FB2_KEY( Section, "date"       );
    FB2_KEY( Section, "epigraph"   );
    FB2_KEY( Section, "poem"       );
    FB2_KEY( Section, "section"    );
    FB2_KEY( Section, "stanza"     );
    FB2_KEY( Section, "subtitle"   );
    FB2_KEY( Section, "title"      );

    FB2_KEY( Anchor, "a"    );
    FB2_KEY( Table, "table" );
    FB2_KEY( Image, "image" );

    FB2_KEY( Parag, "empty-line" );
    FB2_KEY( Parag, "p"          );
    FB2_KEY( Parag, "v"          );

    FB2_KEY( Style,    "style"         );
    FB2_KEY( Strong,   "strong"        );
    FB2_KEY( Emphas,   "emphasis"      );
    FB2_KEY( Strike,   "strikethrough" );
    FB2_KEY( Sub,      "sub"           );
    FB2_KEY( Sup,      "sup"           );
    FB2_KEY( Code,     "code"          );
FB2_END_KEYHASH

Fb2Handler::BodyHandler::BodyHandler(Fb2HtmlWriter &writer, const QString &name, const QXmlAttributes &attributes, const QString &tag, const QString &style)
    : BaseHandler(writer, name)
    , m_parent(NULL)
    , m_tag(tag)
    , m_style(style)
{
    Init(attributes);
}

Fb2Handler::BodyHandler::BodyHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &attributes, const QString &tag, const QString &style)
    : BaseHandler(parent->m_writer, name)
    , m_parent(parent)
    , m_tag(tag)
    , m_style(style)
{
    Init(attributes);
}

void Fb2Handler::BodyHandler::Init(const QXmlAttributes &attributes)
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

Fb2Handler::BaseHandler * Fb2Handler::BodyHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
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
    return new BodyHandler(this, name, attributes, tag, style);
}

void Fb2Handler::BodyHandler::TxtTag(const QString &text)
{
    m_writer.writeCharacters(text);
}

void Fb2Handler::BodyHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_tag.isEmpty()) return;
    if (m_tag == "div") m_writer.writeCharacters(" ");
    m_writer.writeEndElement();
}

bool Fb2Handler::BodyHandler::isNotes() const
{
    if (m_style == "notes") return true;
    return m_parent ? m_parent->isNotes() : false;
}

//---------------------------------------------------------------------------
//  Fb2Handler::AnchorHandler
//---------------------------------------------------------------------------

Fb2Handler::AnchorHandler::AnchorHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &attributes)
    : BodyHandler(parent, name, attributes, "a")
{
    QString href = Value(attributes, "href");
    m_writer.writeAttribute("href", href);
}

//---------------------------------------------------------------------------
//  Fb2Handler::ImageHandler
//---------------------------------------------------------------------------

Fb2Handler::ImageHandler::ImageHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &attributes)
    : BodyHandler(parent, name, attributes, "img")
{
    QString href = Value(attributes, "href");
    while (href.left(1) == "#") href.remove(0, 1);
    QString path = m_writer.getFile(href);
    m_writer.writeAttribute("src", path);
    m_writer.writeAttribute("alt", href);
}

//---------------------------------------------------------------------------
//  Fb2Handler::BinaryHandler
//---------------------------------------------------------------------------

Fb2Handler::BinaryHandler::BinaryHandler(Fb2HtmlWriter &writer, const QString &name, const QXmlAttributes &attributes)
    : BaseHandler(writer, name)
    , m_file(Value(attributes, "id"))
{
}

void Fb2Handler::BinaryHandler::TxtTag(const QString &text)
{
    m_text += text;
}

void Fb2Handler::BinaryHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    QByteArray in; in.append(m_text);
    if (!m_file.isEmpty()) m_writer.addFile(m_file, QByteArray::fromBase64(in));
}


//---------------------------------------------------------------------------
//  Fb2Handler
//---------------------------------------------------------------------------

Fb2Handler::Fb2Handler(Fb2ReadThread &thread)
    : QXmlDefaultHandler()
    , m_writer(thread)
    , m_handler(NULL)
{
    m_writer.setAutoFormatting(true);
}

Fb2Handler::~Fb2Handler()
{
    if (m_handler) delete m_handler;
}

bool Fb2Handler::startElement(const QString & namespaceURI, const QString & localName, const QString &qName, const QXmlAttributes &attributes)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);

    const QString name = qName.toLower();
    if (m_handler) return m_handler->doStart(name, attributes);

    qCritical() << name;

    if (name == "fictionbook") {
        m_handler = new RootHandler(m_writer, name);
        return true;
    } else {
        m_error = QObject::tr("The file is not an FB2 file.");
        return false;
    }
}

static bool isWhiteSpace(const QString &str)
{
    return str.simplified().isEmpty();
}

bool Fb2Handler::characters(const QString &str)
{
    QString s = str.simplified();
    if (s.isEmpty()) return true;
    if (isWhiteSpace(str.left(1))) s.prepend(" ");
    if (isWhiteSpace(str.right(1))) s.append(" ");
    return m_handler && m_handler->doText(s);
}

bool Fb2Handler::endElement(const QString & namespaceURI, const QString & localName, const QString &qName)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);
    bool found = false;
    return m_handler && m_handler->doEnd(qName.toLower(), found);
}

bool Fb2Handler::fatalError(const QXmlParseException &exception)
{
    qCritical() << QObject::tr("Parse error at line %1, column %2: %3")
       .arg(exception.lineNumber())
       .arg(exception.columnNumber())
       .arg(exception.message());
    return false;
}

QString Fb2Handler::errorString() const
{
    return m_error;
}

#undef FB2_BEGIN_KEYHASH

#undef FB2_END_KEYHASH

#undef FB2_KEY
