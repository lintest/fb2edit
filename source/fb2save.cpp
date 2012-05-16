#include <QtGui>
#include <QtDebug>

#include "fb2save.h"
#include "fb2view.h"

//---------------------------------------------------------------------------
//  Fb2SaveWriter
//---------------------------------------------------------------------------

Fb2SaveWriter::Fb2SaveWriter(Fb2WebView &view, QIODevice &device)
    : QXmlStreamWriter(&device)
    , m_view(view)
{
    setAutoFormatting(true);
    setAutoFormattingIndent(2);
    writeStartDocument();
}

Fb2SaveWriter::~Fb2SaveWriter()
{
    writeEndDocument();
}

QString Fb2SaveWriter::getFile(const QString &path)
{
    return QString();
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::BodyHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(Fb2SaveHandler::BodyHandler)
    FB2_KEY( Section , "div"    );
    FB2_KEY( Anchor  , "a"      );
    FB2_KEY( Image   , "img"  );
    FB2_KEY( Table   , "table"  );
    FB2_KEY( Parag   , "p"      );
    FB2_KEY( Strong  , "b"      );
    FB2_KEY( Emphas  , "i"      );
    FB2_KEY( Strike  , "strike" );
    FB2_KEY( Sub     , "sub"    );
    FB2_KEY( Sup     , "sup"    );
    FB2_KEY( Code    , "tt"     );
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
    int count = atts.count();
    for (int i = 0; i < count; i++) {
        QString name = atts.qName(i);
        if (name.left(4) != "fb2:") continue;
        m_writer.writeAttribute(name.mid(4), atts.value(i));
    }
}

Fb2XmlHandler::NodeHandler * Fb2SaveHandler::BodyHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    QString tag, style;
    switch (toKeyword(name)) {
        case Section   : tag = atts.value("class") ; break;
        case Anchor    : return new AnchorHandler(this, name, atts);
        case Image     : return new ImageHandler(this, name, atts);
        case Parag     : return new ParagHandler(this, name, atts);
        case Strong    : tag = "strong"        ; break;
        case Emphas    : tag = "emphasis"      ; break;
        case Strike    : tag = "strikethrough" ; break;
        case Code      : tag = "code"          ; break;
        case Sub       : tag = "sub"           ; break;
        case Sup       : tag = "sup"           ; break;
        default: ;
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
    m_writer.writeEndElement();
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::RootHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::RootHandler::RootHandler(Fb2SaveWriter &writer, const QString &name, const QXmlAttributes &atts)
    : BodyHandler(writer, name, atts, "FictionBook")
{
    m_writer.writeAttribute("xmlns", "http://www.gribuser.ru/xml/fictionbook/2.0");
    m_writer.writeAttribute("xmlns:l", "http://www.w3.org/1999/xlink");
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::AnchorHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::AnchorHandler::AnchorHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &atts)
    : BodyHandler(parent, name, atts, "a")
{
    QString href = Value(atts, "href");
    m_writer.writeAttribute("l:href", href);
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::ImageHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::ImageHandler::ImageHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &atts)
    : BodyHandler(parent, name, atts, "image")
{
    QString href = Value(atts, "href");
    QString path = m_writer.getFile(href);
    m_writer.writeAttribute("l:href", path);
    m_writer.writeEndElement();
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::ParagHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::ParagHandler::ParagHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &atts)
    : BodyHandler(parent, name, atts, "")
    , m_parent(parent->tag())
    , m_empty(true)
{
}

Fb2XmlHandler::NodeHandler * Fb2SaveHandler::ParagHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    if (m_empty) start();
    return BodyHandler::NewTag(name, atts);
}

void Fb2SaveHandler::ParagHandler::TxtTag(const QString &text)
{
    if (m_empty) {
        if (isWhiteSpace(text)) return;
        start();
    }
    BodyHandler::TxtTag(text);
}

void Fb2SaveHandler::ParagHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_empty) m_writer.writeStartElement("empty-line");
    m_writer.writeEndElement();
}

void Fb2SaveHandler::ParagHandler::start()
{
    if (!m_empty) return;
    QString tag = m_parent == "stanza" ? "v" : "p";
    m_writer.writeStartElement(tag);
    m_empty = false;
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::Fb2SaveHandler(Fb2WebView &view, QIODevice &device)
    : Fb2XmlHandler()
    , m_writer(view, device)
{
}

Fb2XmlHandler::NodeHandler * Fb2SaveHandler::CreateRoot(const QString &name, const QXmlAttributes &atts)
{
    if (name == "body") return new RootHandler(m_writer, name, atts);
    m_error = QObject::tr("The tag <body> was not found.");
    return 0;
}

