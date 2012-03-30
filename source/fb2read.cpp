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
    , m_document(NULL)
    , m_abort(false)
{
}

Fb2ReadThread::~Fb2ReadThread()
{
    stop();
    wait();
    if (m_document) delete m_document;
}

QTextDocument * Fb2ReadThread::doc()
{
    QMutexLocker locker(&mutex);
    Q_UNUSED(locker);
    return m_document ? m_document->clone() : NULL;
}

const QString & Fb2ReadThread::file()
{
    QMutexLocker locker(&mutex);
    Q_UNUSED(locker);
    return m_filename;
}

void Fb2ReadThread::stop()
{
    QMutexLocker locker(&mutex);
    Q_UNUSED(locker);
    m_abort = true;
}

void Fb2ReadThread::run()
{
    QFile file(m_filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCritical() << tr("Cannot read file %1:\n%2.").arg(m_filename).arg(file.errorString());
        return;
    }

    m_document = new QTextDocument();
    Fb2Handler handler(*m_document);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    QXmlInputSource source(&file);

    bool ok = reader.parse(source);

    m_document->moveToThread(QApplication::instance()->thread());

    if (ok) emit sendDocument();
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

Fb2Handler::RootHandler::RootHandler(QTextDocument &document, const QString &name)
    : BaseHandler(name)
    , m_document(document)
    , m_cursor(&document, false)
    , m_empty(true)
{
    m_cursor.beginEditBlock();
}

Fb2Handler::RootHandler::~RootHandler()
{
    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(6);
    blockFormat.setBottomMargin(6);

    m_cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    m_cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    m_cursor.mergeBlockFormat(blockFormat);

    m_cursor.endEditBlock();
}

Fb2Handler::BaseHandler * Fb2Handler::RootHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    switch (toKeyword(name)) {
        case Body   : { BaseHandler * handler = new BodyHandler(m_cursor, name); m_empty = false; return handler; }
        case Descr  : return new DescrHandler(m_cursor, name);
        case Binary : return new BinaryHandler(m_document, name, attributes);
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
        case Title   : return new HeaderHandler(*this, name);
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

Fb2Handler::BaseHandler * Fb2Handler::TextHandler::NewTag(const QString & name, const QXmlAttributes &attributes)
{
    return BaseHandler::NewTag(name, attributes);
}

void Fb2Handler::TextHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    cursor().setCharFormat(m_charFormat);
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
    , m_frame(cursor.currentFrame())
    , m_feed(false)
{
    QTextFrameFormat frameFormat;
    frameFormat.setBorder(1);
    frameFormat.setPadding(8);
    frameFormat.setTopMargin(4);
    frameFormat.setBottomMargin(4);
    cursor.insertFrame(frameFormat);
}

Fb2Handler::BaseHandler * Fb2Handler::BodyHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    switch (toKeyword(name)) {
        case Paragraph : return new BlockHandler(*this, name, attributes);
        case Image     : return new ImageHandler(*this, name, attributes);
        case Section   : return new SectionHandler(*this, name, attributes);
        case Title     : return new TitleHandler(*this, name, attributes);
        case Poem      : return new SectionHandler(*this, name, attributes);
        case Stanza    : return new SectionHandler(*this, name, attributes);
        default: return NULL;
    }
}

void Fb2Handler::BodyHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_frame) cursor().setPosition(m_frame->lastPosition());
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
}

Fb2Handler::BaseHandler * Fb2Handler::SectionHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    Keyword keyword = toKeyword(name);

    switch (keyword) {
        case Paragraph:
        case Emptyline:
        case Image:
        case Title:
            if (m_feed) cursor().insertBlock();
            m_feed = true;
            break;
        default:
            m_feed = false;
    }

    switch (keyword) {
        case Emptyline : return new BaseHandler(name); break;
        case Paragraph : return new BlockHandler(*this, name, attributes); break;
        case Section   : return new SectionHandler(*this, name, attributes); break;
        case Title     : return new TitleHandler(*this, name, attributes); break;
        case Poem      : return new PoemHandler(*this, name, attributes); break;
        case Image     : return new ImageHandler(*this, name, attributes); break;
        default        : return new UnknowHandler(*this, name); break;
    }
;
}

void Fb2Handler::SectionHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_frame) cursor().setPosition(m_frame->lastPosition());
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

    QTextTableFormat tableFormat;
    tableFormat.setBorder(0);
    tableFormat.setCellPadding(4);
    tableFormat.setCellSpacing(0);
    tableFormat.setWidth(QTextLength(QTextLength::PercentageLength, 100));
    m_table = cursor().insertTable(1, 1, tableFormat);

    QTextTableCellFormat cellFormat;
    cellFormat.setBackground(Qt::darkGreen);
    cellFormat.setForeground(Qt::white);
    m_table->cellAt(cursor()).setFormat(cellFormat);

    QTextBlockFormat blockFormat;
    blockFormat.setAlignment(Qt::AlignHCenter);
    cursor().mergeBlockFormat(blockFormat);
}

Fb2Handler::BaseHandler * Fb2Handler::TitleHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    if (m_feed) cursor().insertBlock(); else m_feed = true;

    switch (toKeyword(name)) {
        case Paragraph : return new BlockHandler(*this, name, attributes); break;
        case Emptyline : return new BlockHandler(*this, name, attributes); break;
        default        : return new UnknowHandler(*this, name); break;
    }
}

void Fb2Handler::TitleHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_frame) cursor().setPosition(m_frame->lastPosition());
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

Fb2Handler::BaseHandler * Fb2Handler::PoemHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    switch (toKeyword(name)) {
        case Title:
        case Epigraph:
        case Stanza:
        case Author:
            if (m_feed) m_table->appendRows(1);
            m_feed = true;
            return new StanzaHandler(*this, name, attributes);
        default:
            return NULL;
    }
}

void Fb2Handler::PoemHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_frame) cursor().setPosition(m_frame->lastPosition());
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

Fb2Handler::BaseHandler * Fb2Handler::StanzaHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
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
        case Subtitle : return new TitleHandler(*this, name, attributes);
        case Verse    : return new BlockHandler(*this, name, attributes);
        default: return NULL;
    }
}

//---------------------------------------------------------------------------
//  Fb2Handler::BlockHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(BlockHandler)
    insert("strong"        , Strong);
    insert("emphasis"      , Emphasis);
    insert("style"         , Style);
    insert("a"             , Anchor);
    insert("strikethrough" , Strike);
    insert("sub"           , Sub);
    insert("sup"           , Sup);
    insert("code"          , Code);
    insert("image"         , Image);
FB2_END_KEYHASH

Fb2Handler::BlockHandler::BlockHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes, QTextCharFormat * format)
    : TextHandler(parent, name)
{
    Q_UNUSED(attributes);
    if (format) cursor().mergeCharFormat(*format);
}

Fb2Handler::BaseHandler * Fb2Handler::BlockHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(attributes);
    QTextCharFormat format;
    switch (toKeyword(name)) {
        case Image    : return new ImageHandler(*this, name, attributes);
        case Strong   : format.setFontWeight(QFont::Bold);  break;
        case Emphasis : format.setFontItalic(true); break;
        case Strike   : format.setFontStrikeOut(true); break;
        case Sub      : format.setVerticalAlignment(QTextCharFormat::AlignSubScript); break;
        case Sup      : format.setVerticalAlignment(QTextCharFormat::AlignSuperScript); break;
        case Anchor   : {
            QString href = Value(attributes, "href");
            if (!href.isEmpty()) {
                format.setAnchorHref(href);
                format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
            }
        } break;
        default:
            return new UnknowHandler(*this, name); break;
    }
    return new BlockHandler(*this, name, attributes, &format);
}

void Fb2Handler::BlockHandler::TxtTag(const QString &text)
{
    cursor().insertText(text);
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

Fb2Handler::BaseHandler * Fb2Handler::ImageHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    Q_UNUSED(name);
    Q_UNUSED(attributes);
    return false;
}

//---------------------------------------------------------------------------
//  Fb2Handler::UnknowHandler
//---------------------------------------------------------------------------

Fb2Handler::UnknowHandler::UnknowHandler(TextHandler &parent, const QString &name)
    : TextHandler(parent, name)
    , m_parent(parent)
{
}

Fb2Handler::BaseHandler * Fb2Handler::UnknowHandler::NewTag(const QString &name, const QXmlAttributes &attributes)
{
    return m_parent.NewTag(name, attributes);
}

void Fb2Handler::UnknowHandler::TxtTag(const QString &text)
{
    cursor().insertText(text);
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

void Fb2Handler::BinaryHandler::TxtTag(const QString &text)
{
    m_text += text;
}

void Fb2Handler::BinaryHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    QByteArray in; in.append(m_text);
    QImage img = QImage::fromData(QByteArray::fromBase64(in));
    if (!m_file.isEmpty()) m_document.addResource(QTextDocument::ImageResource, QUrl(m_file), img);
}

//---------------------------------------------------------------------------
//  Fb2Handler
//---------------------------------------------------------------------------

Fb2Handler::Fb2Handler(QTextDocument & document)
    : m_document(document)
    , m_handler(NULL)
{
    document.clear();
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

#undef FB2_BEGIN_KEYHASH

#undef FB2_END_KEYHASH
