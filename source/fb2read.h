#ifndef FB2READ_H
#define FB2READ_H

#include <QXmlDefaultHandler>
#include <QTextCursor>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QTextEdit;
QT_END_NAMESPACE

class Fb2Handler : public QXmlDefaultHandler
{
public:
    Fb2Handler(QTextEdit * editor);
    virtual ~Fb2Handler();
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &exception);
    QString errorString() const;

private:
    enum DocSection {
        None   = 0x0000,
        Body   = 0x0001,
        Descr  = 0x0002,
        Bynary = 0x0003,
    };

    enum DocKeyword {
        Image,
        Paragraph,
        Section,
        Title,
    };

    class SectionHash : public QHash<QString, DocSection> { public: SectionHash(); };

    class KeywordHash : public QHash<QString, DocKeyword> { public: KeywordHash(); };

    static DocSection GetSection(const QString &name);

    static DocKeyword GetKeyword(const QString &name);

private:
    bool BodyNew(const QString &name, const QXmlAttributes &attributes);
    bool BodyEnd(const QString &name);

private:
    QTextEdit * m_editor;
    QTextCursor m_cursor;
    QString m_text;
    QString m_error;
    QStringList m_tags;
    DocSection m_section;
    QList<QTextFrame*> m_frames;
};

#endif // FB2READ_H
