#ifndef FB2READ_H
#define FB2READ_H

#include <QXmlDefaultHandler>
#include <QTextCursor>
#include <QStringList>
#include <QTextFrameFormat>
#include <QTextBlockFormat>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE
class QTextEdit;
QT_END_NAMESPACE

#define FB2_BEGIN_KEYLIST private: enum Keyword {

#define FB2_END_KEYLIST None }; \
class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); }; \
static Keyword toKeyword(const QString &name); private:

class Fb2Handler : public QXmlDefaultHandler
{
public:
    Fb2Handler(QTextDocument & document);
    virtual ~Fb2Handler();
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &exception);
    QString errorString() const;

private:
    class ContentHandler
    {
    public:
        ContentHandler(Fb2Handler &owner, const QString &name);
        ContentHandler(ContentHandler &parent, const QString &name);
        virtual ~ContentHandler();
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doText(const QString &text);
        virtual bool doEnd(const QString &name, bool & exit);
        QTextDocument & document() { return m_owner.document(); }
        QTextCursor & cursor() { return m_owner.cursor(); }
    protected:
        void CloseHandler(QTextCursor &cursor);
        Fb2Handler & m_owner;
        ContentHandler * m_handler;
        const QString m_name;
    };

    class RootHandler : public ContentHandler
    {
        FB2_BEGIN_KEYLIST
            Style,
            Descr,
            Body,
            Binary,
        FB2_END_KEYLIST
    public:
        RootHandler(Fb2Handler & owner, const QString &name);
        virtual bool doStart(const QString & name, const QXmlAttributes &attributes);
    private:
    };

    class DescrHandler : public ContentHandler
    {
    public:
        DescrHandler(ContentHandler &parent, const QString &name) : ContentHandler(parent, name) {}
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doEnd(const QString &name, bool & exit);
    };

    class BodyHandler : public ContentHandler
    {
        FB2_BEGIN_KEYLIST
            Image,
            Title,
            Epigraph,
            Section,
            Paragraph,
            Poem,
            Stanza,
            Verse,
       FB2_END_KEYLIST
    public:
        BodyHandler(ContentHandler &parent, const QString &name);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
    private:
        bool m_feed;
    };

    class TextHandler : public ContentHandler
    {
        FB2_BEGIN_KEYLIST
            Strong,
            Emphasis,
            Style,
            Anchor,
            Strikethrough,
            Sub,
            Sup,
            Code,
            Image,
        FB2_END_KEYLIST
    public:
        TextHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doText(const QString &text);
    };

    class ImageHandler : public ContentHandler
    {
    public:
        ImageHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
    };

    class SectionHandler : public ContentHandler
    {
        FB2_BEGIN_KEYLIST
            Title,
            Epigraph,
            Image,
            Annotation,
            Section,
            Paragraph,
            Poem,
            Subtitle,
            Cite,
            Emptyline,
            Table,
        FB2_END_KEYLIST
    public:
        SectionHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doEnd(const QString &name, bool & exit);
    private:
        QTextFrame * m_frame;
        bool m_feed;
    };

    class TitleHandler : public ContentHandler
    {
        FB2_BEGIN_KEYLIST
            Paragraph,
            Emptyline,
        FB2_END_KEYLIST
    public:
        TitleHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doEnd(const QString &name, bool & exit);
    private:
        QTextFrame * m_frame;
        QTextTable * m_table;
        bool m_feed;
    };

    class PoemHandler : public ContentHandler
    {
        FB2_BEGIN_KEYLIST
            Title,
            Epigraph,
            Stanza,
            Author,
            Date,
        FB2_END_KEYLIST
    public:
        PoemHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doEnd(const QString &name, bool & exit);
    private:
        QTextFrame * m_frame;
        QTextTable * m_table;
        bool m_feed;
    };

    class StanzaHandler : public ContentHandler
    {
        FB2_BEGIN_KEYLIST
            Title,
            Subtitle,
            Verse,
        FB2_END_KEYLIST
    public:
        StanzaHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
    private:
        bool m_feed;
    };

    class BinaryHandler : public ContentHandler
    {
    public:
        BinaryHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doText(const QString &text);
        virtual bool doEnd(const QString &name, bool & exit);
    private:
        QString m_file;
        QString m_text;
    };

public:
    bool setHandler(ContentHandler * handler) { m_handler = handler; return handler; }
    QTextDocument & document() { return m_document; }
    QTextCursor & cursor() { return m_cursor; }

private:
    QTextDocument & m_document;
    QTextCursor m_cursor;
    ContentHandler * m_handler;
    QString m_error;
};

#endif // FB2READ_H
