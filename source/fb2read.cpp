#include <QtGui>
#include <QTextEdit>

#include "fb2read.h"

Fb2Reader::Fb2Reader(QTextEdit * editor)
    : m_editor(editor)
    , m_cursor(editor->textCursor())
    , m_section(None)
{
    m_cursor.beginEditBlock();
    editor->clear();
    m_cursor.movePosition(QTextCursor::Start);
}

Fb2Reader::~Fb2Reader()
{
    m_cursor.endEditBlock();
}

bool Fb2Reader::startElement(const QString & /* namespaceURI */,
                               const QString & /* localName */,
                               const QString &qName,
                               const QXmlAttributes &attributes)
{
    const QString name = qName.toLower();

    switch (m_tags.count()) {
        case 0: {
            if (name != "fictionbook") {
                m_error = QObject::tr("The file is not an FB2 file.");
                return false;
            };
        } break;
        case 1: {
            if (name == "body") {
                m_section = Body;
            } else if (name == "description") {
                m_section = Descr;
            } else if (name == "binary") {
                m_section = Bynary;
            } else {
                m_section = None;
            }
        } break;
        default: {
            if (name == "p") m_cursor.insertBlock();
        } break;
    }

    m_text.clear();

    m_tags << name;

    return true;
}

bool Fb2Reader::endElement(const QString & /* namespaceURI */,
                             const QString & /* localName */,
                             const QString &qName)
{
    const QString name = qName.toLower();
    int index = m_tags.lastIndexOf(name);
    int count = m_tags.count();
    for (int i = index; i < count; i++) m_tags.removeLast();
    if (m_tags.count() < 2) m_section = None;

    return true;
}

bool Fb2Reader::characters(const QString &str)
{
    switch (m_section) {
        case Body: {
            m_cursor.insertText(str);
        } break;
        default: {
            m_text += str;
        }
    }
    return true;
}

bool Fb2Reader::fatalError(const QXmlParseException &exception)
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

QString Fb2Reader::errorString() const
{
    return m_error;
}

/*

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
