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
        ContentHandler(Fb2Handler &owner);
        ContentHandler(ContentHandler &parent);
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
        QTextFrame * m_frame;
    };

    class RootHandler : public ContentHandler
    {
    public:
        RootHandler(Fb2Handler & owner);
        virtual bool doStart(const QString & name, const QXmlAttributes &attributes);
    private:
        enum Keyword {
            None = 0,
            Style,
            Descr,
            Body,
            Binary,
        };
        class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); };
        static Keyword toKeyword(const QString & name);
    };

    class DescrHandler : public ContentHandler
    {
    public:
        DescrHandler(ContentHandler &parent) : ContentHandler(parent) {}
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doEnd(const QString &name, bool & exit);
    };

    class BodyHandler : public ContentHandler
    {
    public:
        BodyHandler(ContentHandler &parent);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
    private:
        enum Keyword {
            None = 0,
            Image,
            Title,
            Epigraph,
            Section,
            Paragraph,
            Poem,
            Stanza,
            Verse,

        };
        class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); };
        static Keyword toKeyword(const QString &name);
    private:
        bool m_feed;
    };

    class TextHandler : public ContentHandler
    {
    public:
        TextHandler(ContentHandler &parent, const QString &name);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doText(const QString &text);
    private:
        enum Keyword {
            None = 0,
            Strong,
            Emphasis,
            Style,
            Anchor,
            Strikethrough,
            Sub,
            Sup,
            Code,
            Image,
        };
        class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); };
        static Keyword toKeyword(const QString &name);
    private:
        QString m_name;
    };

    class ImageHandler : public ContentHandler
    {
    public:
        ImageHandler(ContentHandler &parent, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
    };

    class SectionHandler : public ContentHandler
    {
    public:
        SectionHandler(ContentHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
    private:
        enum Keyword {
            None = 0,
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
        };
        class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); };
        static Keyword toKeyword(const QString &name);
    private:
        QString m_name;
        bool m_feed;
    };

    class PoemHandler : public ContentHandler
    {
    public:
        PoemHandler(ContentHandler &parent, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
    private:
        enum Keyword {
            None = 0,
            Title,
            Epigraph,
            Stanza,
            Author,
            Date,
        };
        class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); };
        static Keyword toKeyword(const QString &name);
    private:
        QTextTable * m_table;
        bool m_feed;
    };

    class StanzaHandler : public ContentHandler
    {
    public:
        StanzaHandler(ContentHandler &parent, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
    private:
        enum Keyword {
            None = 0,
            Title,
            Subtitle,
            Verse,
        };
        class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); };
        static Keyword toKeyword(const QString &name);
    private:
        bool m_feed;
    };

    class BinaryHandler : public ContentHandler
    {
    public:
        BinaryHandler(ContentHandler &parent, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doText(const QString &text);
        virtual bool doEnd(const QString &name, bool & exit);
    private:
        QString m_name;
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
