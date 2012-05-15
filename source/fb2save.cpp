#include <QtGui>
#include <QtDebug>

#include "fb2save.h"

//---------------------------------------------------------------------------
//  Fb2SaveThread
//---------------------------------------------------------------------------

Fb2SaveThread::Fb2SaveThread(QObject *parent, const QString &filename)
    : QThread(parent)
    , m_filename(filename)
    , m_abort(false)
{
}

Fb2SaveThread::~Fb2SaveThread()
{
    stop();
    wait();
}

void Fb2SaveThread::stop()
{
    QMutexLocker locker(&mutex);
    Q_UNUSED(locker);
    m_abort = true;
}

void Fb2SaveThread::run()
{
    if (parse()) emit html(m_filename, m_html);
}

void Fb2SaveThread::onFile(const QString &name, const QString &path)
{
    emit file(name, path);
}

bool Fb2SaveThread::parse()
{
    QFile file(m_filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCritical() << QObject::tr("Cannot read file %1: %2.").arg(m_filename).arg(file.errorString());
        return false;
    }
    Fb2SaveHandler handler(*this);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    QXmlInputSource source(&file);
    return reader.parse(source);
}

//---------------------------------------------------------------------------
//  Fb2SaveWriter
//---------------------------------------------------------------------------

Fb2SaveWriter::Fb2SaveWriter(Fb2SaveThread &thread)
    : QXmlStreamWriter(thread.data())
    , m_thread(thread)
    , m_id(0)
{
    setAutoFormatting(true);
    setAutoFormattingIndent(2);
}

QString Fb2SaveWriter::addFile(const QString &name, const QByteArray &data)
{
    QString path = getFile(name);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        m_thread.onFile(name, path);
    }
    return path;
}

QString Fb2SaveWriter::getFile(const QString &name)
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

QString Fb2SaveWriter::newId()
{
    return QString("FB2E%1").arg(++m_id);
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::BodyHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(Fb2SaveHandler::BodyHandler)
    FB2_KEY( Section , "div"           );
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

    FB2_KEY( Strong  , "b"             );
    FB2_KEY( Emphas  , "i"             );
    FB2_KEY( Strike  , "strike"        );
    FB2_KEY( Sub     , "sub"           );
    FB2_KEY( Sup     , "sup"           );
    FB2_KEY( Code    , "tt"            );
FB2_END_KEYHASH

Fb2SaveHandler::BodyHandler::BodyHandler(Fb2SaveWriter &writer, const QString &name, const QXmlAttributes &atts, const QString &tag, const QString &style)
    : NodeHandler(name)
    , m_writer(writer)
    , m_tag(tag)
    , m_style(style)
{
    Init(atts);
}

Fb2SaveHandler::BodyHandler::BodyHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag, const QString &style)
    : NodeHandler(name)
    , m_writer(parent->m_writer)
    , m_tag(tag)
    , m_style(style)
{
    Init(atts);
}

void Fb2SaveHandler::BodyHandler::Init(const QXmlAttributes &atts)
{
    if (m_tag.isEmpty()) return;
    m_writer.writeStartElement(m_tag);
/*
    QString id = Value(atts, "id");
    if (!id.isEmpty()) {
        if (m_style == "section" && isNotes()) m_style = "note";
        m_writer.writeAttribute("id", id);
    } else if (m_tag == "div" || m_tag == "img") {
        m_writer.writeAttribute("id", m_writer.newId());
    }
    if (!m_style.isEmpty()) {
        if (m_style == "body" && Value(atts, "name").toLower() == "notes") m_style = "notes";
        m_writer.writeAttribute("class", m_style);
    }
*/
}

Fb2XmlHandler::NodeHandler * Fb2SaveHandler::BodyHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    QString tag, style;
    switch (toKeyword(name)) {
        case Anchor    : return new AnchorHandler(this, name, atts);
        case Image     : return new ImageHandler(this, name, atts);
        case Section   : tag = "div"; style = name; break;
        case Parag     : tag = "p";   break;

        case Strong    : tag = "strong"        ; break;
        case Emphas    : tag = "emphasis"      ; break;
        case Strike    : tag = "strikethrough" ; break;
        case Code      : tag = "cide"          ; break;
        case Sub       : tag = "sub"           ; break;
        case Sup       : tag = "sup"           ; break;
    }
    return new BodyHandler(this, name, atts, tag, style);
}

void Fb2SaveHandler::BodyHandler::TxtTag(const QString &text)
{
    m_writer.writeCharacters(text);
}

void Fb2SaveHandler::BodyHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_tag.isEmpty()) return;
    if (m_tag == "div") m_writer.writeCharacters(" ");
    m_writer.writeEndElement();
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::AnchorHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::AnchorHandler::AnchorHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &atts)
    : BodyHandler(parent, name, atts, "a")
{
    QString href = Value(atts, "href");
    m_writer.writeAttribute("href", href);
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::ImageHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::ImageHandler::ImageHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &atts)
    : BodyHandler(parent, name, atts, "img")
{
    QString href = Value(atts, "href");
    while (href.left(1) == "#") href.remove(0, 1);
    QString path = m_writer.getFile(href);
    m_writer.writeAttribute("src", path);
    m_writer.writeAttribute("alt", href);
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::Fb2SaveHandler(Fb2SaveThread &thread)
    : Fb2XmlHandler()
    , m_writer(thread)
{
}

Fb2XmlHandler::NodeHandler * Fb2SaveHandler::CreateRoot(const QString &name, const QXmlAttributes &atts)
{
    if (name == "body") return new BodyHandler(m_writer, name, atts, "fictionbook");
    m_error = QObject::tr("The tag <body> was not found.");
    return 0;
}

