#include <QtGui>
#include <QTextEdit>
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

QString Fb2ReadThread::file()
{
    QMutexLocker locker(&mutex);
    Q_UNUSED(locker);
    return m_filename;
}

QString Fb2ReadThread::html()
{
    QMutexLocker locker(&mutex);
    Q_UNUSED(locker);
    return m_html;
}

void Fb2ReadThread::stop()
{
    QMutexLocker locker(&mutex);
    Q_UNUSED(locker);
    m_abort = true;
}

void Fb2ReadThread::run()
{
    if (Fb2Handler::toHTML(m_filename, m_html)) {
        emit sendDocument();
    }
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
    m_handler = new BaseHandler(name);
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

Fb2Handler::RootHandler::RootHandler(QXmlStreamWriter &writer, const QString &name)
    : BaseHandler(name)
    , m_writer(writer)
{
    m_writer.writeStartElement("html");
    m_writer.writeStartElement("body");
}

Fb2Handler::RootHandler::~RootHandler()
{
    m_writer.writeEndElement();
    m_writer.writeEndElement();
}

Fb2Handler::BaseHandler * Fb2Handler::RootHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    switch (toKeyword(name)) {
        case Body   : return new BodyHandler(m_writer, name, attributes, "div", name);
        case Descr  : return new DescrHandler(m_writer, name);
        case Binary : return new BinaryHandler(name, attributes);
        default: return NULL;
    }
}

//---------------------------------------------------------------------------
//  Fb2Handler::DescrHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(DescrHandler)
    insert( "title-info"   , Title   );
    insert( "publish-info" , Publish );
FB2_END_KEYHASH

Fb2Handler::BaseHandler * Fb2Handler::DescrHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(attributes);
    switch (toKeyword(name)) {
        case Title   : return new HeaderHandler(m_writer, name);
        case Publish : return new BaseHandler(name);
        default: return NULL;
    }
}

//---------------------------------------------------------------------------
//  Fb2Handler::HeaderHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(HeaderHandler)
    insert( "book-title"   , Title    );
    insert( "author"       , Author   );
    insert( "sequence"     , Sequence );
    insert( "genre"        , Genre    );
    insert( "lang"         , Lang     );
    insert( "annotation"   , Annot    );
    insert( "coverpage"    , Cover    );
FB2_END_KEYHASH

Fb2Handler::BaseHandler * Fb2Handler::HeaderHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(attributes);
    switch (toKeyword(name)) {
        case Title   : return new BaseHandler(name);
        case Annot   : return new BaseHandler(name);
        default: return NULL;
    }
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

Fb2Handler::BodyHandler::BodyHandler(QXmlStreamWriter &writer, const QString &name, const QXmlAttributes &attributes, const QString &tag, const QString &style)
    : BaseHandler(name)
    , m_writer(writer)
{
    m_writer.writeStartElement(tag);
    QString id = Value(attributes, "id");
    if (!id.isEmpty()) m_writer.writeAttribute("id", id);
    if (!style.isEmpty()) m_writer.writeAttribute("style", style);
}

Fb2Handler::BodyHandler::~BodyHandler()
{
    m_writer.writeEndElement();
}

Fb2Handler::BaseHandler * Fb2Handler::BodyHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    switch (toKeyword(name)) {
        case Section   : return new BodyHandler(m_writer, name, attributes, "div", name);
        case Anchor    : return new AnchorHandler(m_writer, name, attributes);
        case Image     : return new ImageHandler(m_writer, name, attributes);
        case Parag     : return new BodyHandler(m_writer, name, attributes, "p");
        case Strong    : return new BodyHandler(m_writer, name, attributes, "b");
        case Emphas    : return new BodyHandler(m_writer, name, attributes, "i");
        case Strike    : return new BodyHandler(m_writer, name, attributes, "s");
        case Code      : return new BodyHandler(m_writer, name, attributes, "tt");
        case Sub       : return new BodyHandler(m_writer, name, attributes, "sub");
        case Sup       : return new BodyHandler(m_writer, name, attributes, "sup");
        default: return NULL;
    }
}

void Fb2Handler::BodyHandler::TxtTag(const QString &text)
{
    m_writer.writeCharacters(text);
}

//---------------------------------------------------------------------------
//  Fb2Handler::AnchorHandler
//---------------------------------------------------------------------------

Fb2Handler::AnchorHandler::AnchorHandler(QXmlStreamWriter &writer, const QString &name, const QXmlAttributes &attributes)
    : BodyHandler(writer, name, attributes, "a")
{
    QString href = Value(attributes, "href");
    writer.writeAttribute("href", href);
}

//---------------------------------------------------------------------------
//  Fb2Handler::ImageHandler
//---------------------------------------------------------------------------

Fb2Handler::ImageHandler::ImageHandler(QXmlStreamWriter &writer, const QString &name, const QXmlAttributes &attributes)
    : BodyHandler(writer, name, attributes, "img")
{
    QString href = Value(attributes, "href");
    writer.writeAttribute("src", href);
}

//---------------------------------------------------------------------------
//  Fb2Handler::BinaryHandler
//---------------------------------------------------------------------------

Fb2Handler::BinaryHandler::BinaryHandler(const QString &name, const QXmlAttributes &attributes)
    : BaseHandler(name)
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
    QImage img = QImage::fromData(QByteArray::fromBase64(in));
//    if (!m_file.isEmpty()) m_document.addResource(QTextDocument::ImageResource, QUrl(m_file), img);
}

//---------------------------------------------------------------------------
//  Fb2Handler
//---------------------------------------------------------------------------

Fb2Handler::Fb2Handler(QString &string)
    : QXmlDefaultHandler()
    , m_writer(&string)
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
    qCritical() << QObject::tr("Parse error at line %1, column %2:\n%3")
       .arg(exception.lineNumber())
       .arg(exception.columnNumber())
       .arg(exception.message());
    return false;
}

QString Fb2Handler::errorString() const
{
    return m_error;
}

bool Fb2Handler::toHTML(const QString &filename, QString &html)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCritical() << QObject::tr("Cannot read file %1:\n%2.").arg(filename).arg(file.errorString());
        return false;
    }

    Fb2Handler handler(html);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    QXmlInputSource source(&file);
    return reader.parse(source);
}

#undef FB2_BEGIN_KEYHASH

#undef FB2_END_KEYHASH

#undef FB2_KEY
