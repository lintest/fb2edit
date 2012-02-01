#include <QtGui>
#include <QTextEdit>
#include <QtDebug>

#include "fb2read.h"

#define FB2_BEGIN_KEYHASH(x) \
Fb2Handler::x::Keyword Fb2Handler::x::toKeyword(const QString &name) \
{                                                                    \
    static const KeywordHash map;                                    \
    KeywordHash::const_iterator i = map.find(name);                  \
    return i == map.end() ? None : i.value();                        \
}                                                                    \
Fb2Handler::x::KeywordHash::KeywordHash() {

#define FB2_END_KEYHASH }

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


//---------------------------------------------------------------------------
//  Fb2Handler::ContentHandler
//---------------------------------------------------------------------------

Fb2Handler::ContentHandler::ContentHandler(Fb2Handler &owner, const QString &name)
    : m_owner(owner)
    , m_handler(NULL)
    , m_name(name)
{
}

Fb2Handler::ContentHandler::ContentHandler(ContentHandler &parent, const QString &name)
    : m_owner(parent.m_owner)
    , m_handler(NULL)
    , m_name(name)
{
}

Fb2Handler::ContentHandler::~ContentHandler()
{
    if (m_handler) delete m_handler;
}

bool Fb2Handler::ContentHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);
    m_handler = new ContentHandler(*this, name);
    return true;
}

bool Fb2Handler::ContentHandler::doText(const QString &text)
{
    if (m_handler) return m_handler->doText(text);
    return true;
}

bool Fb2Handler::ContentHandler::doEnd(const QString &name, bool & exit)
{
    if (m_handler) {
        bool ok = m_handler->doEnd(name, exit);
        if (exit) {
            delete m_handler;
            m_handler = NULL;
            exit = false;
        }
        return ok;
    } else {
        if (name != m_name) qCritical() << QObject::tr("Conglict XML tags: <%1> - </%2>").arg(m_name).arg(name);
        exit = true;
        return true;
    }
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

Fb2Handler::RootHandler::RootHandler(Fb2Handler & owner, const QString &name)
    : ContentHandler(owner, name)
{
}

bool Fb2Handler::RootHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);

    switch (toKeyword(name)) {
        case Descr  : return m_handler = new DescrHandler(*this, name);
        case Body   : return m_handler = new BodyHandler(*this, name);
        case Binary : return m_handler = new BinaryHandler(*this, name, attributes);
        default:
            qCritical() << QObject::tr("Unknown XML tag: %1").arg(name);
            return m_handler = new ContentHandler(*this, name);
    }

    return false;
}

//---------------------------------------------------------------------------
//  Fb2Handler::DescrHandler
//---------------------------------------------------------------------------

bool Fb2Handler::DescrHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(name);
    Q_UNUSED(attributes);
    return true;
}

bool Fb2Handler::DescrHandler::doEnd(const QString &name, bool & exit)
{
    Q_UNUSED(name);
    if (name == "description") exit = true;
    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::BodyHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(BodyHandler)
    insert("image",    Image);
    insert("title",    Title);
    insert("epigraph", Epigraph);
    insert("section",  Section);
    insert("p",        Paragraph);
    insert("poem",     Poem);
    insert("stanza",   Stanza);
    insert("v",        Verse);
FB2_END_KEYHASH

Fb2Handler::BodyHandler::BodyHandler(ContentHandler &parent, const QString &name)
    : ContentHandler(parent, name)
    , m_feed(true)
{
    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(4);
    blockFormat.setBottomMargin(4);
    cursor().setBlockFormat(blockFormat);
}

bool Fb2Handler::BodyHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);

    switch (toKeyword(name)) {
        case Paragraph : m_handler = new TextHandler(*this, name, attributes); break;
        case Image     : m_handler = new ImageHandler(*this, name, attributes); break;
        case Section   : m_handler = new SectionHandler(*this, name, attributes); break;
        case Title     : m_handler = new TitleHandler(*this, name, attributes); break;
        case Poem      : m_handler = new SectionHandler(*this, name, attributes); break;
        case Stanza    : m_handler = new SectionHandler(*this, name, attributes); break;
        default        : m_handler = new ContentHandler(*this, name); break;
    }

    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::SectionHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(SectionHandler)
    insert("title",      Title);
    insert("epigraph",   Epigraph);
    insert("image",      Image);
    insert("annotation", Annotation);
    insert("section",    Section);
    insert("p",          Paragraph);
    insert("poem",       Poem);
    insert("subtitle",   Subtitle);
    insert("cite",       Cite);
    insert("empty-line", Emptyline);
    insert("table",      Table);
FB2_END_KEYHASH

Fb2Handler::SectionHandler::SectionHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : ContentHandler(parent, name)
    , m_frame(cursor().currentFrame())
    , m_feed(false)
{
    Q_UNUSED(attributes);

    QTextFrameFormat frameFormat;
    frameFormat.setBorder(1);
    frameFormat.setPadding(8);
    frameFormat.setTopMargin(4);
    frameFormat.setBottomMargin(4);
    if (name == "title") frameFormat.setWidth(QTextLength(QTextLength::PercentageLength, 80));
    cursor().insertFrame(frameFormat);

    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(4);
    blockFormat.setBottomMargin(4);

    cursor().setBlockFormat(blockFormat);
}

bool Fb2Handler::SectionHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);

    Keyword keyword = toKeyword(name);

    switch (keyword) {
        case Paragraph:
        case Emptyline:
        case Image:
            if (m_feed) cursor().insertBlock();
            m_feed = true;
            break;
        default:
            m_feed = false;
    }

    switch (keyword) {
        case Emptyline : m_handler = new ContentHandler(*this, name); break;
        case Paragraph : m_handler = new TextHandler(*this, name, attributes); break;
        case Section   : m_handler = new SectionHandler(*this, name, attributes); break;
        case Title     : m_handler = new TitleHandler(*this, name, attributes); break;
        case Poem      : m_handler = new PoemHandler(*this, name, attributes); break;
        case Image     : m_handler = new ImageHandler(*this, name, attributes); break;
        default        : m_handler = new ContentHandler(*this, name); break;
    }

    return true;
}

bool Fb2Handler::SectionHandler::doEnd(const QString &name, bool & exit)
{
    bool ok = ContentHandler::doEnd(name, exit);
    if (exit && m_frame) cursor().setPosition(m_frame->lastPosition());
    return ok;
}

//---------------------------------------------------------------------------
//  Fb2Handler::TitleHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(TitleHandler)
    insert("p",          Paragraph);
    insert("empty-line", Emptyline);
FB2_END_KEYHASH

Fb2Handler::TitleHandler::TitleHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : ContentHandler(parent, name)
    , m_frame(cursor().currentFrame())
    , m_table(NULL)
    , m_feed(false)
{
    Q_UNUSED(attributes);
    QTextTableFormat format;
    format.setBorder(0);
    format.setCellPadding(4);
    format.setCellSpacing(4);
    m_table = cursor().insertTable(1, 1, format);
}

bool Fb2Handler::TitleHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);

    if (m_feed) cursor().insertBlock(); else m_feed = true;

    switch (toKeyword(name)) {
        case Paragraph : m_handler = new TextHandler(*this, name, attributes); break;
        case Emptyline : m_handler = new TextHandler(*this, name, attributes); break;
        default: m_handler = new ContentHandler(*this, name); break;
    }

    return true;
}

bool Fb2Handler::TitleHandler::doEnd(const QString &name, bool & exit)
{
    bool ok = ContentHandler::doEnd(name, exit);
    if (exit && m_frame) cursor().setPosition(m_frame->lastPosition());
    return ok;
}

//---------------------------------------------------------------------------
//  Fb2Handler::PoemHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(PoemHandler)
    insert("title",      Title);
    insert("epigraph",   Epigraph);
    insert("stanza",     Stanza);
    insert("author",     Author);
    insert("date",       Date);
FB2_END_KEYHASH

Fb2Handler::PoemHandler::PoemHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : ContentHandler(parent, name)
    , m_frame(cursor().currentFrame())
    , m_table(NULL)
    , m_feed(false)
{
    Q_UNUSED(attributes);
    QTextTableFormat format;
    format.setBorder(1);
    format.setCellPadding(4);
    format.setCellSpacing(4);
    format.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    m_table = cursor().insertTable(1, 1, format);
}

bool Fb2Handler::PoemHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);

    Keyword keyword = toKeyword(name);

    switch (keyword) {
        case Title:
        case Epigraph:
        case Stanza:
        case Author:
            if (m_feed) m_table->appendRows(1);
            m_feed = true;
            m_handler = new StanzaHandler(*this, name, attributes);
            break;
        default:
            m_handler = new ContentHandler(*this, name);
    }

    return true;
}

bool Fb2Handler::PoemHandler::doEnd(const QString &name, bool & exit)
{
    bool ok = ContentHandler::doEnd(name, exit);
    if (exit && m_frame) cursor().setPosition(m_frame->lastPosition());
    return ok;
}

//---------------------------------------------------------------------------
//  Fb2Handler::StanzaHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(StanzaHandler)
    insert("title",      Title);
    insert("subtitle",   Subtitle);
    insert("v",          Verse);
FB2_END_KEYHASH

Fb2Handler::StanzaHandler::StanzaHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : ContentHandler(parent, name)
    , m_feed(false)
{
    Q_UNUSED(attributes);
}

bool Fb2Handler::StanzaHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);

    Keyword keyword = toKeyword(name);

    switch (keyword) {
        case Title:
        case Subtitle:
        case Verse:
            if (m_feed) cursor().insertBlock();
            m_feed = true;
        default: ;
    }

    switch (keyword) {
        case Title:
        case Subtitle:
            m_handler = new TitleHandler(*this, name, attributes); break;
        case Verse:
            m_handler = new TextHandler(*this, name, attributes); break;
        default:
            m_handler = new ContentHandler(*this, name); break;
    }

    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::TextHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(TextHandler)
    insert("strong"        , Strong);
    insert("emphasis"      , Emphasis);
    insert("style"         , Style);
    insert("a"             , Anchor);
    insert("strikethrough" , Strikethrough);
    insert("sub"           , Sub);
    insert("sup"           , Sup);
    insert("code"          , Code);
    insert("image"         , Image);
FB2_END_KEYHASH

Fb2Handler::TextHandler::TextHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : ContentHandler(parent, name)
{
    Q_UNUSED(attributes);
    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(4);
    blockFormat.setBottomMargin(4);
    cursor().setBlockFormat(blockFormat);
}

bool Fb2Handler::TextHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(attributes);
    if (m_handler) return m_handler->doStart(name, attributes);
    switch (toKeyword(name)) {
        default: m_handler = new ContentHandler(*this, name); break;
    }
    return true;
}

bool Fb2Handler::TextHandler::doText(const QString &text)
{
    if (m_handler) return m_handler->doText(text);
    cursor().insertText(text);
    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::ImageHandler
//---------------------------------------------------------------------------

Fb2Handler::ImageHandler::ImageHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : ContentHandler(parent, name)
{
    QString image = Value(attributes, "href");
    while (image.left(1) == "#") image.remove(0, 1);
    if (!image.isEmpty()) cursor().insertImage(image);
}

bool Fb2Handler::ImageHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(name);
    Q_UNUSED(attributes);
    return false;
}

//---------------------------------------------------------------------------
//  Fb2Handler::BinaryHandler
//---------------------------------------------------------------------------

Fb2Handler::BinaryHandler::BinaryHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : ContentHandler(parent, name)
    , m_file(Value(attributes, "id"))
{
}

bool Fb2Handler::BinaryHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(name);
    Q_UNUSED(attributes);
    return false;
}

bool Fb2Handler::BinaryHandler::doText(const QString &text)
{
    m_text += text;
    return true;
}

bool Fb2Handler::BinaryHandler::doEnd(const QString &name, bool & exit)
{
    Q_UNUSED(name);
    QByteArray in; in.append(m_text);
    QImage img = QImage::fromData(QByteArray::fromBase64(in));
    if (!m_file.isEmpty()) document().addResource(QTextDocument::ImageResource, QUrl(m_file), img);
    exit = true;
    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler
//---------------------------------------------------------------------------

Fb2Handler::Fb2Handler(QTextDocument & document)
    : m_document(document)
    , m_cursor(&document)
    , m_handler(NULL)
{
    m_cursor.beginEditBlock();
    document.clear();
    m_cursor.movePosition(QTextCursor::Start);
}

Fb2Handler::~Fb2Handler()
{
    if (m_handler) delete m_handler;
    m_cursor.endEditBlock();
}

bool Fb2Handler::startElement(const QString & namespaceURI, const QString & localName, const QString &qName, const QXmlAttributes &attributes)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);

    const QString name = qName.toLower();
    if (m_handler) return m_handler->doStart(name, attributes);

    if (name == "fictionbook") {
        m_handler = new RootHandler(*this, name);
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
    bool exit = false;
    return m_handler && m_handler->doEnd(qName.toLower(), exit);
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
