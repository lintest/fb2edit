#ifndef FB2READ_H
#define FB2READ_H

#include <QXmlDefaultHandler>
#include <QTextCursor>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QTextEdit;
QT_END_NAMESPACE

class Fb2Reader : public QXmlDefaultHandler
{
public:
    enum DocumentSection {
        None   = 0x0000,
        Body   = 0x0001,
        Descr  = 0x0002,
        Bynary = 0x0003,
    };
    Fb2Reader(QTextEdit * editor);
    virtual ~Fb2Reader();
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &exception);
    QString errorString() const;
private:
    QTextEdit * m_editor;
    QTextCursor m_cursor;
    QString m_text;
    QString m_error;
    QStringList m_tags;
    DocumentSection m_section;
};

#endif // FB2READ_H
