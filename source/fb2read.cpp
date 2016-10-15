#include "fb2read.hpp"

#include <QtDebug>

#include "fb2imgs.hpp"
#include "fb2xml2.h"

//---------------------------------------------------------------------------
//  FbReadThread
//---------------------------------------------------------------------------

void FbReadThread::execute(QObject *parent, QXmlInputSource *source, QIODevice *device)
{
    FbReadThread *thread = new FbReadThread(parent, source, device);
    connect(thread, SIGNAL(html(QString, FbStore*)), parent, SLOT(html(QString, FbStore*)));
    thread->start();
}

FbReadThread::FbReadThread(QObject *parent, QXmlInputSource *source, QIODevice *device)
    : QThread(parent)
    , m_device(device)
    , m_source(source)
{
    m_store = new FbStore(this);
}

FbReadThread::~FbReadThread()
{
    if (m_source) delete m_source;
    if (m_device) delete m_device;
}

void FbReadThread::run()
{
    if (parse()) {
        emit html(m_html, m_store);
    } else {
        delete m_store;
    }
    deleteLater();
}

bool FbReadThread::parse()
{
    QXmlStreamWriter writer(&m_html);
    FbReadHandler handler(writer);

    connect(&handler, SIGNAL(binary(QString,QByteArray)), m_store, SLOT(binary(QString,QByteArray)));
    connect(&handler, SIGNAL(warning(int,int,QString)), parent(), SIGNAL(warning(int,int,QString)));
    connect(&handler, SIGNAL(error(int,int,QString)), parent(), SIGNAL(error(int,int,QString)));
    connect(&handler, SIGNAL(fatal(int,int,QString)), parent(), SIGNAL(fatal(int,int,QString)));

#ifdef FB2_USE_LIBXML2
    XML2::XmlReader reader;
#else
    QXmlSimpleReader reader;
#endif

    reader.setContentHandler(&handler);
    reader.setLexicalHandler(&handler);
    reader.setErrorHandler(&handler);

#ifdef FB2_USE_LIBXML2
    if (m_device) {
        return reader.parse(m_device);
    } else {
        return reader.parse(m_source);
    }
#else
    if (m_device) {
        m_source = new QXmlInputSource();
        m_source->setData(m_device->readAll());
    }
    return reader.parse(m_source);
#endif
}

/*
FbReadThread::FbReadThread(QObject *parent, const QString &filename, const QString &xml)
    : QThread(parent)
    , m_temp(0)
    , m_filename(filename)
    , m_xml(xml)
    , m_abort(false)
{
    connect(this, SIGNAL(html(QString)), parent, SLOT(html(QString)));
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
    connect(&handler, SIGNAL(binary(QString,QByteArray)), this, SLOT(data(QString,QByteArray)));
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

*/

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
    , m_head(true)
{
}

FbXmlHandler::NodeHandler * FbReadHandler::RootHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    switch (toKeyword(name)) {
        case Binary: return new BinaryHandler(m_owner, name, atts);
        case Style: return new StyleHandler(m_owner, name, m_style);
        default: ;
    }

    if (m_head) {
        writeHeader();
        m_head = false;
    }

    return new TextHandler(m_owner, name, atts, "fb:" + name);
}

void FbReadHandler::RootHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (!m_head) writer().writeEndElement();
}

void FbReadHandler::RootHandler::writeScript(const QString &src)
{
    writer().writeStartElement("script");
    writer().writeAttribute("type", "text/javascript");
    writer().writeAttribute("src", src);
    writer().writeCharacters("");
    writer().writeEndElement();
}

void FbReadHandler::RootHandler::writeHeader()
{
    writer().writeStartElement("head");

    writeScript("qrc:/js/jquery.js");
    writeScript("qrc:/js/location.js");
    if (!m_style.isEmpty()) {
        writer().writeStartElement("style");
        writer().writeAttribute("type", "text/css");
        writer().writeAttribute("id", "origin");
        writer().writeCharacters(m_style);
        writer().writeEndElement();
    }

    writer().writeEndElement();

    writer().writeStartElement("body");
}

//---------------------------------------------------------------------------
//  FbReadHandler::StyleHandler
//---------------------------------------------------------------------------

FbReadHandler::StyleHandler::StyleHandler(FbReadHandler &owner, const QString &name, QString &text)
    : BaseHandler(owner, name)
    , m_text(text)
{
}

void FbReadHandler::StyleHandler::TxtTag(const QString &text)
{
    m_text += text;
}

//---------------------------------------------------------------------------
//  FbReadHandler::TextHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(FbReadHandler::TextHandler)
    FB2_KEY( Anchor  , "a"             );
    FB2_KEY( Image   , "image"         );
    FB2_KEY( Origin  , "table"         );
    FB2_KEY( Origin  , "td"            );
    FB2_KEY( Origin  , "th"            );
    FB2_KEY( Origin  , "tr"            );

    FB2_KEY( Parag   , "empty-line"    );
    FB2_KEY( Parag   , "text-author"   );
    FB2_KEY( Parag   , "subtitle"      );
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

FbReadHandler::TextHandler::TextHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts, const QString &tag)
    : BaseHandler(owner, name)
    , m_parent(NULL)
    , m_tag(tag)
    , m_empty(true)
{
    Init(name, atts);
}

FbReadHandler::TextHandler::TextHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag)
    : BaseHandler(parent->m_owner, name)
    , m_parent(parent)
    , m_tag(tag)
    , m_empty(true)
{
    Init(name, atts);
}

void FbReadHandler::TextHandler::Init(const QString &name, const QXmlAttributes &atts)
{
    Keyword key = toKeyword(name);
    writer().writeStartElement(m_tag);
    int count = atts.count();
    for (int i = 0; i < count; ++i) {
        QString name = atts.qName(i);
        switch (key) {
            case Anchor: { if (atts.localName(i) == "href") name = "href"; break; }
            case Image:  { if (atts.localName(i) == "href") name = "src"; break; }
            default: ;
        }
        writer().writeAttribute(name, atts.value(i));
    }
    if (m_tag == "p" && (name == "text-author" || name == "subtitle")) {
        writer().writeAttribute("fb:class", name);
    }
}

FbXmlHandler::NodeHandler * FbReadHandler::TextHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    m_empty = false;
    QString tag;
    switch (toKeyword(name)) {
        case Origin : tag = name;   break;
        case Anchor : tag = "a";    break;
        case Image  : tag = "img";  break;
        case Parag  : tag = "p";    break;
        case Strong : tag = "b";    break;
        case Emphas : tag = "i";    break;
        case Strike : tag = "s";    break;
        case Code   : tag = "tt";   break;
        case Sub    : tag = "sub";  break;
        case Sup    : tag = "sup";  break;
        case Style  : tag = "span"; break;
        default     : tag = "fb:" + name;
    }
    return new TextHandler(this, name, atts, tag);
}

void FbReadHandler::TextHandler::TxtTag(const QString &text)
{
    writer().writeCharacters(text);
    m_empty = false;
}

void FbReadHandler::TextHandler::EndTag(const QString &name)
{
    if (m_empty) {
        if (name == "p") {
            writer().writeEmptyElement("br");
        } else {
            writer().writeCharacters("");
        }
    }
    writer().writeEndElement();
}

bool FbReadHandler::TextHandler::isNotes() const
{
    if (m_style == "notes") return true;
    return m_parent ? m_parent->isNotes() : false;
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

bool FbReadHandler::load(QObject *page, QXmlInputSource &source, QString &html)
{
    QXmlStreamWriter writer(&html);
    FbReadHandler handler(writer);

    connect(&handler, SIGNAL(binary(QString,QByteArray)), page, SLOT(binary(QString,QByteArray)));

#ifdef FB2_USE_LIBXML2
    XML2::XmlReader reader;
#else
    QXmlSimpleReader reader;
#endif

    reader.setContentHandler(&handler);
    reader.setLexicalHandler(&handler);
    reader.setErrorHandler(&handler);

    return reader.parse(source);
}

FbReadHandler::FbReadHandler(QXmlStreamWriter &writer)
    : FbXmlHandler()
    , m_writer(writer)
{
    m_writer.setAutoFormatting(true);
    m_writer.setAutoFormattingIndent(2);
    m_writer.writeStartElement("html");
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
    emit binary(name, data);
}
