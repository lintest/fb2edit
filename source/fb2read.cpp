#include <QtGui>
#include <QTextEdit>
#include <QtDebug>

#include "fb2read.h"

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

Fb2Handler::ContentHandler::ContentHandler(Fb2Handler &owner)
    : m_owner(owner)
    , m_handler(NULL)
    , m_frame(NULL)
{
}

Fb2Handler::ContentHandler::ContentHandler(ContentHandler &parent)
    : m_owner(parent.m_owner)
    , m_handler(NULL)
    , m_frame(NULL)
{
}

Fb2Handler::ContentHandler::~ContentHandler()
{
    if (m_handler) delete m_handler;
}

bool Fb2Handler::ContentHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);
    m_handler = new ContentHandler(*this);
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
        if (m_frame) cursor().setPosition(m_frame->lastPosition());
        exit = true;
        return true;
    }
}

//---------------------------------------------------------------------------
//  Fb2Handler::RootHandler
//---------------------------------------------------------------------------

Fb2Handler::RootHandler::KeywordHash::KeywordHash()
{
    insert("stylesheet", Style);
    insert("description", Descr);
    insert("body", Body);
    insert("binary", Binary);
}

Fb2Handler::RootHandler::RootHandler(Fb2Handler & owner)
    : ContentHandler(owner)
{
}

Fb2Handler::RootHandler::Keyword Fb2Handler::RootHandler::toKeyword(const QString &name)
{
    static KeywordHash map;
    KeywordHash::const_iterator i = map.find(name);
    return i == map.end() ? None : i.value();
}

bool Fb2Handler::RootHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);
    switch (toKeyword(name)) {
        case Descr  : return m_handler = new DescrHandler(*this);
        case Body   : return m_handler = new BodyHandler(*this);
        case Binary : return m_handler = new BinaryHandler(*this, attributes);
    }
    qCritical() << QObject::tr("Unknown XML tag: %1").arg(name);
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

Fb2Handler::BodyHandler::KeywordHash::KeywordHash()
{
    insert("image",    Image);
    insert("title",    Title);
    insert("epigraph", Epigraph);
    insert("section",  Section);

    insert("p",        Paragraph);
    insert("poem",     Poem);
    insert("stanza",   Stanza);
    insert("v",        Verse);
}

Fb2Handler::BodyHandler::BodyHandler(ContentHandler &parent)
    : ContentHandler(parent)
    , m_empty(true)
{
    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(4);
    blockFormat.setBottomMargin(4);
    cursor().setBlockFormat(blockFormat);
}

Fb2Handler::BodyHandler::Keyword Fb2Handler::BodyHandler::toKeyword(const QString &name)
{
    static KeywordHash map;
    KeywordHash::const_iterator i = map.find(name);
    return i == map.end() ? None : i.value();
}

bool Fb2Handler::BodyHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);

    switch (toKeyword(name)) {
        case Paragraph : m_handler = new TextHandler(*this, name); break;
        case Image     : m_handler = new ImageHandler(*this, attributes); break;
        case Section   : m_handler = new SectionHandler(*this, name, attributes); break;
        case Title     : m_handler = new SectionHandler(*this, name, attributes); break;
        case Poem      : m_handler = new SectionHandler(*this, name, attributes); break;
        case Stanza    : m_handler = new SectionHandler(*this, name, attributes); break;
        default        : m_handler = new TextHandler(*this, name); break;
    }

    return true;
}

bool Fb2Handler::BodyHandler::doText(const QString &text)
{
    if (m_handler) return m_handler->doText(text);
    cursor().insertText(text);
    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::SectionHandler
//---------------------------------------------------------------------------

Fb2Handler::SectionHandler::KeywordHash::KeywordHash()
{
    insert("image",   Image);
    insert("section", Section);
    insert("title",   Title);

    insert("p",       Paragraph);
    insert("poem",    Poem);
    insert("stanza",  Stanza);
    insert("v",       Verse);
}

Fb2Handler::SectionHandler::SectionHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes)
    : ContentHandler(parent)
    , m_name(name)
    , m_empty(true)
{
    m_frame = cursor().currentFrame();
    QTextFrameFormat frameFormat;
    frameFormat.setBorder(1);
    frameFormat.setPadding(8);
    frameFormat.setTopMargin(4);
    frameFormat.setBottomMargin(4);
    cursor().insertFrame(frameFormat);

    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(4);
    blockFormat.setBottomMargin(4);
    cursor().setBlockFormat(blockFormat);
}

Fb2Handler::SectionHandler::Keyword Fb2Handler::SectionHandler::toKeyword(const QString &name)
{
    static KeywordHash map;
    KeywordHash::const_iterator i = map.find(name);
    return i == map.end() ? None : i.value();
}

bool Fb2Handler::SectionHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);
    switch (toKeyword(name)) {
        case Paragraph : m_handler = new TextHandler(*this, name); break;
        case Image     : m_handler = new ImageHandler(*this, attributes); break;
        case Section   : m_handler = new SectionHandler(*this, name, attributes); break;
        case Title     : m_handler = new SectionHandler(*this, name, attributes); break;
        case Poem      : m_handler = new SectionHandler(*this, name, attributes); break;
        case Stanza    : m_handler = new SectionHandler(*this, name, attributes); break;
        default        : m_handler = new TextHandler(*this, name); break;
    }
    return true;
}

bool Fb2Handler::SectionHandler::doText(const QString &text)
{
    if (m_handler) return m_handler->doText(text);
    cursor().insertText(text);
    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::TextHandler
//---------------------------------------------------------------------------

Fb2Handler::TextHandler::TextHandler(ContentHandler &parent, const QString &name)
    : ContentHandler(parent)
    , m_name(name)
{
    QTextBlockFormat blockFormat;
    blockFormat.setTopMargin(4);
    blockFormat.setBottomMargin(4);
    cursor().setBlockFormat(blockFormat);
}

Fb2Handler::TextHandler::KeywordHash::KeywordHash()
{
    insert("strong"        , Strong);
    insert("emphasis"      , Emphasis);
    insert("style"         , Style);
    insert("a"             , Anchor);
    insert("strikethrough" , Strikethrough);
    insert("sub"           , Sub);
    insert("sup"           , Sup);
    insert("code"          , Code);
    insert("image"         , Image);
}

Fb2Handler::TextHandler::Keyword Fb2Handler::TextHandler::toKeyword(const QString &name)
{
    static KeywordHash map;
    KeywordHash::const_iterator i = map.find(name);
    return i == map.end() ? None : i.value();
}

bool Fb2Handler::TextHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);
    switch (toKeyword(name)) {
        default        : m_handler = new ContentHandler(*this); break;
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

Fb2Handler::ImageHandler::ImageHandler(ContentHandler &parent, const QXmlAttributes &attributes)
    : ContentHandler(parent)
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

Fb2Handler::BinaryHandler::BinaryHandler(ContentHandler &parent, const QXmlAttributes &attributes)
    : ContentHandler(parent), m_name(Value(attributes, "id"))
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
    if (!m_name.isEmpty()) m_owner.document().addResource(QTextDocument::ImageResource, QUrl(m_name), img);
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
        m_handler = new RootHandler(*this);
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

/*

    QMessageBox::information(
        m_editor->window(),
        QObject::tr("fb2edit"),
        QObject::tr("%1=%2\n%1=%3")
            .arg(attributes.localName(i))
            .arg(attributes.value(i))
            .arg(image)
        );


//! [2]
    QTextCursor cursor(editor->textCursor());
    cursor.movePosition(QTextCursor::Start);
//! [2] //! [3]
    QTextFrame *topFrame = cursor.currentFrame();
    QTextFrameFormat topFrameFormat = topFrame->frameFormat();
    topFrameFormat.setPadding(16);
    topFrame->setFrameFormat(topFrameFormat);

    QTextCharFormat textFormat;
    QTextCharFormat boldFormat;
    boldFormat.setFontWeight(QFont::Bold);

    QTextFrameFormat referenceFrameFormat;
    referenceFrameFormat.setBorder(1);
    referenceFrameFormat.setPadding(8);
    referenceFrameFormat.setPosition(QTextFrameFormat::FloatRight);
    referenceFrameFormat.setWidth(QTextLength(QTextLength::PercentageLength, 40));
    cursor.insertFrame(referenceFrameFormat);

    cursor.insertText("A company", boldFormat);
    cursor.insertBlock();
    cursor.insertText("321 City Street");
    cursor.insertBlock();
    cursor.insertText("Industry Park");
    cursor.insertBlock();
    cursor.insertText("Another country");
//! [3]

//! [4]
    cursor.setPosition(topFrame->lastPosition());

    cursor.insertText(name, textFormat);
    QString line;
    foreach (line, address.split("\n")) {
        cursor.insertBlock();
        cursor.insertText(line);
    }
//! [4] //! [5]
    cursor.insertBlock();
    cursor.insertBlock();

    QDate date = QDate::currentDate();
    cursor.insertText(tr("Date: %1").arg(date.toString("d MMMM yyyy")),
                      textFormat);
    cursor.insertBlock();

    QTextFrameFormat bodyFrameFormat;
    bodyFrameFormat.setWidth(QTextLength(QTextLength::PercentageLength, 100));
    cursor.insertFrame(bodyFrameFormat);
//! [5]

//! [6]
    cursor.insertText(tr("I would like to place an order for the following "
                         "items:"), textFormat);
    cursor.insertBlock();
//! [6] //! [7]
    cursor.insertBlock();
//! [7]

//! [8]
    QTextTableFormat orderTableFormat;
    orderTableFormat.setAlignment(Qt::AlignHCenter);
    QTextTable *orderTable = cursor.insertTable(1, 2, orderTableFormat);

    QTextFrameFormat orderFrameFormat = cursor.currentFrame()->frameFormat();
    orderFrameFormat.setBorder(1);
    cursor.currentFrame()->setFrameFormat(orderFrameFormat);
//! [8]

//! [9]
    cursor = orderTable->cellAt(0, 0).firstCursorPosition();
    cursor.insertText(tr("Product"), boldFormat);
    cursor = orderTable->cellAt(0, 1).firstCursorPosition();
    cursor.insertText(tr("Quantity"), boldFormat);
//! [9]

//! [10]
    for (int i = 0; i < orderItems.count(); ++i) {
        QPair<QString,int> item = orderItems[i];
        int row = orderTable->rows();

        orderTable->insertRows(row, 1);
        cursor = orderTable->cellAt(row, 0).firstCursorPosition();
        cursor.insertText(item.first, textFormat);
        cursor = orderTable->cellAt(row, 1).firstCursorPosition();
        cursor.insertText(QString("%1").arg(item.second), textFormat);
    }
//! [10]

//! [11]
    cursor.setPosition(topFrame->lastPosition());

    cursor.insertBlock();
//! [11] //! [12]
    cursor.insertText(tr("Please update my records to take account of the "
                         "following privacy information:"));
    cursor.insertBlock();
//! [12]

//! [13]
    QTextTable *offersTable = cursor.insertTable(2, 2);

    cursor = offersTable->cellAt(0, 1).firstCursorPosition();
    cursor.insertText(tr("I want to receive more information about your "
                         "company's products and special offers."), textFormat);
    cursor = offersTable->cellAt(1, 1).firstCursorPosition();
    cursor.insertText(tr("I do not want to receive any promotional information "
                         "from your company."), textFormat);

    if (sendOffers)
        cursor = offersTable->cellAt(0, 0).firstCursorPosition();
    else
        cursor = offersTable->cellAt(1, 0).firstCursorPosition();

    cursor.insertText("X", boldFormat);
//! [13]

//! [14]
    cursor.setPosition(topFrame->lastPosition());
    cursor.insertBlock();
    cursor.insertText(tr("Sincerely,"), textFormat);
    cursor.insertBlock();
    cursor.insertBlock();
    cursor.insertBlock();
    cursor.insertText(name);

    printAction->setEnabled(true);
}
//! [14]

*/
