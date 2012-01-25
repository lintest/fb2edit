#include <QtGui>
#include <QTextEdit>

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
//  Fb2Handler::RootHandler
//---------------------------------------------------------------------------

Fb2Handler::RootHandler::KeywordHash::KeywordHash()
{
    insert("body", Body);
    insert("descriptin", Descr);
    insert("binary", Binary);
}

Fb2Handler::RootHandler::Keyword Fb2Handler::RootHandler::GetKeyword(const QString &name)
{
    static KeywordHash map;
    KeywordHash::const_iterator i = map.find(name);
    return i == map.end() ? None : i.value();
}

bool Fb2Handler::RootHandler::DoStart(const QString &name, const QXmlAttributes &attributes)
{
    switch (GetKeyword(name)) {
        case Descr  : return m_owner.SetHandler(new DescrHandler(this));
        case Body   : return m_owner.SetHandler(new BodyHandler(this));
        case Binary : return m_owner.SetHandler(new ImageHandler(this, attributes));
    }
    return false;
}

//---------------------------------------------------------------------------
//  Fb2Handler::DescrHandler
//---------------------------------------------------------------------------

bool Fb2Handler::DescrHandler::DoStart(const QString &name, const QXmlAttributes &attributes)
{
    return true;
}

bool Fb2Handler::DescrHandler::DoText(const QString &text)
{
    return true;
}

bool Fb2Handler::DescrHandler::DoEnd(const QString &name, bool & exit)
{
    exit = name == "description";
    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::BodyHandler
//---------------------------------------------------------------------------

Fb2Handler::BodyHandler::KeywordHash::KeywordHash()
{
    insert("body",    Body);
    insert("image",   Image);
    insert("p",       Paragraph);
    insert("section", Section);
    insert("title",   Title);
}

Fb2Handler::BodyHandler::Keyword Fb2Handler::BodyHandler::GetKeyword(const QString &name)
{
    static KeywordHash map;
    KeywordHash::const_iterator i = map.find(name);
    return i == map.end() ? None : i.value();
}

bool Fb2Handler::BodyHandler::DoStart(const QString &name, const QXmlAttributes &attributes)
{
    switch (GetKeyword(name)) {
        case Image: {
            QString image = Value(attributes, "href");
            while (image.left(1) == "#") image.remove(0, 1);
            if (!image.isEmpty()) m_cursor.insertImage(image);
        } break;
        case Paragraph: {
            m_cursor.insertBlock();
        } break;
        case Section: {
            m_frames << m_cursor.currentFrame();
            QTextFrameFormat format;
            format.setBorder(1);
            format.setPadding(8);
            m_cursor.insertFrame(format);
        } break;
    }
}

bool Fb2Handler::BodyHandler::DoText(const QString &text)
{
    m_cursor.insertText(text);
    return true;
}

bool Fb2Handler::BodyHandler::DoEnd(const QString &name, bool & exit)
{
    switch (GetKeyword(name)) {
        case Body: {
            exit = true;
        }; break;
        case Section: {
            if (!m_frames.isEmpty()) {
                m_cursor.setPosition(m_frames.last()->lastPosition());
                m_frames.removeLast();
            }
        }; break;
    }
    return true;
}

//---------------------------------------------------------------------------
//  Fb2Handler::ImageHandler
//---------------------------------------------------------------------------

Fb2Handler::ImageHandler::ImageHandler(ContentHandler * parent, const QXmlAttributes &attributes)
    : ContentHandler(parent), m_name(Value(attributes, "id"))
{
}

//---------------------------------------------------------------------------
//  Fb2Handler
//---------------------------------------------------------------------------

Fb2Handler::Fb2Handler(QTextEdit * editor)
    : m_editor(editor)
    , m_cursor(editor->textCursor())
    , m_section(None)
    , m_handler(NULL)
{
    m_cursor.beginEditBlock();
    editor->clear();
    m_cursor.movePosition(QTextCursor::Start);
}

Fb2Handler::~Fb2Handler()
{
    if (m_handler) delete m_handler;
    m_cursor.endEditBlock();
}

bool Fb2Handler::startElement(const QString & /* namespaceURI */,
                               const QString & /* localName */,
                               const QString &qName,
                               const QXmlAttributes &attributes)
{
    const QString name = qName.toLower();
    if (m_handler) return m_handler->DoStart(name, attributes);

    if (name == "fictionbook") {
        m_handler = new RootHandler(*this);
        return true;
    } else {
        m_error = QObject::tr("The file is not an FB2 file.");
        return false;
    }
}

bool Fb2Handler::characters(const QString &str)
{
    return m_handler && m_handler->DoText(str);
}

Fb2Handler::ContentHandler * Fb2Handler::Up(Fb2Handler::ContentHandler * handler)
{
    if (!handler) return NULL;
    ContentHandler * parent = handler->GetParent();
    delete handler;
    return parent;
}

bool Fb2Handler::endElement(const QString & /* namespaceURI */,
                             const QString & /* localName */,
                             const QString &qName)
{
    bool exit = false;
    bool ok = m_handler && m_handler->DoEnd(qName.toLower(), exit);
    if (exit) m_handler = Up(m_handler);
    return ok;
}

bool Fb2Handler::fatalError(const QXmlParseException &exception)
{
    QMessageBox::information(
        m_editor->window(),
        QObject::tr("fb2edit"),
        QObject::tr("Parse error at line %1, column %2:\n%3")
            .arg(exception.lineNumber())
            .arg(exception.columnNumber())
            .arg(exception.message())
        );
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
