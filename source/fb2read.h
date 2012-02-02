#ifndef FB2READ_H
#define FB2READ_H

#include "fb2doc.h"

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
    explicit Fb2Handler(Fb2MainDocument & document);
    virtual ~Fb2Handler();
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &exception);
    QString errorString() const;

private:
    class BaseHandler
    {
    public:
        explicit BaseHandler(const QString &name) : m_name(name), m_handler(NULL) {}
        virtual ~BaseHandler();
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doText(const QString &text);
        virtual bool doEnd(const QString &name, bool & exit);
    protected:
        const QString m_name;
        BaseHandler * m_handler;
    };

    class RootHandler : public BaseHandler
    {
        FB2_BEGIN_KEYLIST
            Style,
            Descr,
            Body,
            Binary,
        FB2_END_KEYLIST
    public:
        explicit RootHandler(Fb2MainDocument &document, const QString &name);
        virtual ~RootHandler();
        virtual bool doStart(const QString & name, const QXmlAttributes &attributes);
    private:
        Fb2MainDocument &m_document;
        QTextCursor m_cursor1;
        QTextCursor m_cursor2;
        bool m_empty;
    };

    class DescrHandler : public BaseHandler
    {
    public:
        explicit DescrHandler(const QString &name) : BaseHandler(name) {}
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doEnd(const QString &name, bool & exit);
    };

    class TextHandler : public BaseHandler
    {
    public:
        explicit TextHandler(QTextCursor &cursor, const QString &name);
        explicit TextHandler(TextHandler &parent, const QString &name);
        bool TextHandler::doEnd(const QString &name, bool & exit);
    protected:
        QTextCursor & cursor() { return m_cursor; }
        QTextCursor & m_cursor;
        QTextBlockFormat m_blockFormat;
        QTextCharFormat m_charFormat;
    };

    class BodyHandler : public TextHandler
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
        explicit BodyHandler(QTextCursor &cursor, const QString &name);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
    private:
        bool m_feed;
    };

    class ImageHandler : public TextHandler
    {
    public:
        explicit ImageHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
    };

    class SectionHandler : public TextHandler
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
        explicit SectionHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doEnd(const QString &name, bool & exit);
    private:
        QTextFrame * m_frame;
        bool m_feed;
    };

    class TitleHandler : public TextHandler
    {
        FB2_BEGIN_KEYLIST
            Paragraph,
            Emptyline,
        FB2_END_KEYLIST
    public:
        explicit TitleHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doEnd(const QString &name, bool & exit);
    private:
        QTextFrame * m_frame;
        QTextTable * m_table;
        bool m_feed;
    };

    class PoemHandler : public TextHandler
    {
        FB2_BEGIN_KEYLIST
            Title,
            Epigraph,
            Stanza,
            Author,
            Date,
        FB2_END_KEYLIST
    public:
        explicit PoemHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doEnd(const QString &name, bool & exit);
    private:
        QTextFrame * m_frame;
        QTextTable * m_table;
        bool m_feed;
    };

    class StanzaHandler : public TextHandler
    {
        FB2_BEGIN_KEYLIST
            Title,
            Subtitle,
            Verse,
        FB2_END_KEYLIST
    public:
        explicit StanzaHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
    private:
        bool m_feed;
    };

    class BlockHandler : public TextHandler
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
        explicit BlockHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doText(const QString &text);
    };

    class BinaryHandler : public BaseHandler
    {
    public:
        explicit BinaryHandler(QTextDocument &document, const QString &name, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doText(const QString &text);
        virtual bool doEnd(const QString &name, bool & exit);
    private:
        QTextDocument & m_document;
        QString m_file;
        QString m_text;
    };

public:
    Fb2MainDocument & document() { return m_document; }

private:
    Fb2MainDocument & m_document;
    RootHandler * m_handler;
    QString m_error;
};

#endif // FB2READ_H
