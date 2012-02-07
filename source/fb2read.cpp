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
//  Fb2Handler::BaseHandler
//---------------------------------------------------------------------------

Fb2Handler::BaseHandler::~BaseHandler()
{
    if (m_handler) delete m_handler;
}

bool Fb2Handler::BaseHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);
    m_handler = new BaseHandler(name);
    return true;
}

bool Fb2Handler::BaseHandler::doText(const QString &text)
{
    if (m_handler) return m_handler->doText(text);
    return true;
}

bool Fb2Handler::BaseHandler::doEnd(const QString &name, bool & exit)
{
    if (m_handler) {
        bool ok = m_handler->doEnd(name, exit);
        if (m_handler->m_closed) {
            delete m_handler;
            m_handler = NULL;
        }
        return ok;
    } else {
        if (name != m_name) qCritical() << QObject::tr("Conglict XML tags: <%1> - </%2>").arg(m_name).arg(name);
        m_closed = true;
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

Fb2Handler::RootHandler::RootHandler(Fb2MainDocument &document, const QString &name)
    : BaseHandler(name)
    , m_document(document)
    , m_cursor1(&document, false)
    , m_cursor2(&document.child(), true)
    , m_empty(true)
{
    m_cursor1.beginEditBlock();
    m_cursor2.beginEditBlock();
}

Fb2Handler::RootHandler::~RootHandler()
{
    m_cursor1.endEditBlock();
    m_cursor2.endEditBlock();
}

bool Fb2Handler::RootHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);

    switch (toKeyword(name)) {
        case Descr  : m_handler = new DescrHandler(name); break;
        case Body   : m_handler = new BodyHandler(m_empty ? m_cursor1 : m_cursor2, name); m_empty = false; break;
        case Binary : m_handler = new BinaryHandler(m_document, name, attributes); break;
        default:
            qCritical() << QObject::tr("Unknown XML tag: %1").arg(name);
            m_handler = new BaseHandler(name);
    }

    return true;
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
    Q_UNUSED(exit);
    if (name == "description") m_closed = true;
    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::TextHandler
//---------------------------------------------------------------------------

Fb2Handler::TextHandler::TextHandler(Fb2TextCursor &cursor, const QString &name)
    : BaseHandler(name)
    , m_cursor(cursor)
    , m_blockFormat(m_cursor.blockFormat())
    , m_charFormat(m_cursor.charFormat())
{
}

Fb2Handler::TextHandler::TextHandler(TextHandler &parent, const QString &name)
    : BaseHandler(name)
    , m_cursor(parent.m_cursor)
    , m_blockFormat(m_cursor.blockFormat())
    , m_charFormat(m_cursor.charFormat())
{
}

bool Fb2Handler::TextHandler::doEnd(const QString &name, bool & exit)
{
    bool ok = BaseHandler::doEnd(name, exit);
    if (!m_handler) cursor().setCharFormat(m_charFormat);
    return ok;
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

Fb2Handler::BodyHandler::BodyHandler(Fb2TextCursor &cursor, const QString &name)
    : TextHandler(cursor, name)
    , m_feed(false)
{
    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(4);
    blockFormat.setBottomMargin(4);
    cursor.setBlockFormat(blockFormat);
}

bool Fb2Handler::BodyHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);

    switch (toKeyword(name)) {
        case Paragraph : m_handler = new BlockHandler(*this, name, attributes); break;
        case Image     : m_handler = new ImageHandler(*this, name, attributes); break;
        case Section   : m_handler = new SectionHandler(*this, name, attributes); break;
        case Title     : m_handler = new TitleHandler(*this, name, attributes); break;
        case Poem      : m_handler = new SectionHandler(*this, name, attributes); break;
        case Stanza    : m_handler = new SectionHandler(*this, name, attributes); break;
        default        : m_handler = new BaseHandler(name); break;
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

Fb2Handler::SectionHandler::SectionHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : TextHandler(parent, name)
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
        case Emptyline : m_handler = new BaseHandler(name); break;
        case Paragraph : m_handler = new BlockHandler(*this, name, attributes); break;
        case Section   : m_handler = new SectionHandler(*this, name, attributes); break;
        case Title     : m_handler = new TitleHandler(*this, name, attributes); break;
        case Poem      : m_handler = new PoemHandler(*this, name, attributes); break;
        case Image     : m_handler = new ImageHandler(*this, name, attributes); break;
        default        : m_handler = new BaseHandler(name); break;
    }

    return true;
}

bool Fb2Handler::SectionHandler::doEnd(const QString &name, bool & exit)
{
    bool ok = BaseHandler::doEnd(name, exit);
    if (m_closed && m_frame) cursor().setPosition(m_frame->lastPosition());
    return ok;
}

//---------------------------------------------------------------------------
//  Fb2Handler::TitleHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(TitleHandler)
    insert("p",          Paragraph);
    insert("empty-line", Emptyline);
FB2_END_KEYHASH

Fb2Handler::TitleHandler::TitleHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : TextHandler(parent, name)
    , m_frame(cursor().currentFrame())
    , m_table(NULL)
    , m_feed(false)
{
    Q_UNUSED(attributes);

    QTextTableFormat format1;
    format1.setBorder(0);
    format1.setCellPadding(4);
    format1.setCellSpacing(4);
    format1.setWidth(QTextLength(QTextLength::PercentageLength, 100));
    m_table = cursor().insertTable(1, 1, format1);

    QTextTableCellFormat format2;
    format2.setBackground(Qt::darkGreen);
    format2.setForeground(Qt::white);
    m_table->cellAt(cursor()).setFormat(format2);
}

bool Fb2Handler::TitleHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);

    if (m_feed) cursor().insertBlock(); else m_feed = true;

    switch (toKeyword(name)) {
        case Paragraph : m_handler = new BlockHandler(*this, name, attributes); break;
        case Emptyline : m_handler = new BlockHandler(*this, name, attributes); break;
        default: m_handler = new BaseHandler(name); break;
    }

    return true;
}

bool Fb2Handler::TitleHandler::doEnd(const QString &name, bool & exit)
{
    bool ok = BaseHandler::doEnd(name, exit);
    if (m_closed && m_frame) cursor().setPosition(m_frame->lastPosition());
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

Fb2Handler::PoemHandler::PoemHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : TextHandler(parent, name)
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
            m_handler = new BaseHandler(name);
    }

    return true;
}

bool Fb2Handler::PoemHandler::doEnd(const QString &name, bool & exit)
{
    bool ok = BaseHandler::doEnd(name, exit);
    if (m_closed && m_frame) cursor().setPosition(m_frame->lastPosition());
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

Fb2Handler::StanzaHandler::StanzaHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : TextHandler(parent, name)
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
            m_handler = new BlockHandler(*this, name, attributes); break;
        default:
            m_handler = new BaseHandler(name); break;
    }

    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::BlockHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(BlockHandler)
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

Fb2Handler::BlockHandler::BlockHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : TextHandler(parent, name)
{
    Q_UNUSED(attributes);
    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(4);
    blockFormat.setBottomMargin(4);
    cursor().setBlockFormat(blockFormat);
}

bool Fb2Handler::BlockHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(attributes);
    if (m_handler) return m_handler->doStart(name, attributes);
    switch (toKeyword(name)) {
        default: m_handler = new BaseHandler(name); break;
    }
    return true;
}

bool Fb2Handler::BlockHandler::doText(const QString &text)
{
    if (m_handler) return m_handler->doText(text);
    cursor().insertText(text);
    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::ImageHandler
//---------------------------------------------------------------------------

Fb2Handler::ImageHandler::ImageHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : TextHandler(parent, name)
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

Fb2Handler::BinaryHandler::BinaryHandler(QTextDocument &document, const QString &name, const QXmlAttributes &attributes)
    : BaseHandler(name)
    , m_document(document)
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
    if (!m_file.isEmpty()) m_document.addResource(QTextDocument::ImageResource, QUrl(m_file), img);
    m_closed = true;
    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler
//---------------------------------------------------------------------------

Fb2Handler::Fb2Handler(Fb2MainDocument & document)
    : m_document(document)
    , m_handler(NULL)
{
    document.clear();
    m_document.child().clear();
}

Fb2Handler::~Fb2Handler()
{
    m_document.clearUndoRedoStacks();
    m_document.child().clearUndoRedoStacks();

    m_document.setModified(false);
    m_document.child().setModified(false);

    if (m_handler) delete m_handler;
}

bool Fb2Handler::startElement(const QString & namespaceURI, const QString & localName, const QString &qName, const QXmlAttributes &attributes)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);

    const QString name = qName.toLower();
    if (m_handler) return m_handler->doStart(name, attributes);

    if (name == "fictionbook") {
        m_handler = new RootHandler(m_document, name);
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

#undef FB2_BEGIN_KEYHASH

#undef FB2_END_KEYHASH
