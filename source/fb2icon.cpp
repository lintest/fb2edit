#include <QApplication>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QVariant>

static const int IconFormatType = 0x1000;

//----------------------------------------------------------------------------
// TextIconFormat
//----------------------------------------------------------------------------

class TextIconFormat : public QTextCharFormat
{
public:
    static void TextIconFormat::insert(QTextCursor &cursor, const QString &name, const QString &text);

    TextIconFormat(const QString &iconName, const QString &text);

    enum Property {
            IconName = QTextFormat::UserProperty + 1,
            IconText = QTextFormat::UserProperty + 2
    };
};

TextIconFormat::TextIconFormat(const QString &iconName, const QString &text)
{
    setObjectType(IconFormatType);
    QTextFormat::setProperty(IconName, iconName);
    QTextFormat::setProperty(IconText, text);
}

void TextIconFormat::insertIcon(QTextCursor &cursor, const QString &name, const QString &text)
{
    QTextCharFormat format = cursor.charFormat();
    TextIconFormat icon(iconName, iconText);
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), icon);
    cursor.setCharFormat(format);
}

//----------------------------------------------------------------------------
// IconTextObjectInterface
//----------------------------------------------------------------------------

class TextIconHandler : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    static void TextIconHandler::install(QTextDocument *doc);
    TextIconHandler(QObject *parent = 0): QObject(parent) {}
    virtual QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format);
    virtual void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format);
};

QSizeF TextIconHandler::intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(doc);
    Q_UNUSED(posInDocument);

    const QTextCharFormat charFormat = format.toCharFormat();
    return IconsetFactory::iconPixmap(charFormat.stringProperty(TextIconFormat::IconName)).size();
}

void TextIconHandler::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(doc);
    Q_UNUSED(posInDocument);

    const QTextCharFormat charFormat = format.toCharFormat();
    const QPixmap pixmap = IconsetFactory::iconPixmap(charFormat.stringProperty(TextIconFormat::IconName));
    painter->drawPixmap(rect, pixmap, pixmap.rect());
}

void TextIconHandler::install(QTextDocument *doc)
{
    static TextIconHandler *handler = 0;
    if (!handler) handler = new TextIconHandler(qApp);
    doc->documentLayout()->registerHandler(IconFormatType, handler);
}
