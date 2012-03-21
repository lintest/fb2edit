#ifndef FB2READ_H
#define FB2READ_H

#include "fb2doc.h"

#include <QMutex>
#include <QThread>
#include <QXmlDefaultHandler>
#include <QTextCursor>
#include <QStringList>
#include <QTextFrameFormat>
#include <QTextBlockFormat>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE
class QTextEdit;
QT_END_NAMESPACE

class Fb2MainDocumen;

class Fb2ReadThread : public QThread
{
    Q_OBJECT

public:
    Fb2ReadThread(QObject *parent, const QString &filename);
    ~Fb2ReadThread();
    void Read(const QString &filename);
    QTextDocument * doc();
    const QString & file();

signals:
    void sendDocument();

public slots:
    void stop();

protected:
    void run();

private:
    const QString m_filename;
    QTextDocument * m_document;
    bool m_abort;
    QMutex mutex;
};


#define FB2_BEGIN_KEYLIST private: enum Keyword {

#define FB2_END_KEYLIST None }; \
class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); }; \
static Keyword toKeyword(const QString &name); private:

class Fb2Handler : public QXmlDefaultHandler
{
public:
    explicit Fb2Handler(QTextDocument & document);
    virtual ~Fb2Handler();
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &exception);
    QString errorString() const;

private:
    class Fb2TextCursor : public QTextCursor
    {
    public:
        explicit Fb2TextCursor(QTextDocument *document, bool foot)
            : QTextCursor(document), m_foot(foot) {}
        bool foot()
            { return m_foot; }
    private:
        const bool m_foot;
    };

    class BaseHandler
    {
    public:
        explicit BaseHandler(const QString &name) : m_name(name), m_handler(NULL), m_closed(false) {}
        virtual ~BaseHandler();
        bool doStart(const QString &name, const QXmlAttributes &attributes);
        bool doText(const QString &text);
        bool doEnd(const QString &name, bool & found);
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes)
            { Q_UNUSED(name); Q_UNUSED(attributes); return NULL; }
        virtual void TxtTag(const QString &text)
            { Q_UNUSED(text); }
        virtual void EndTag(const QString &name)
            { Q_UNUSED(name); }
    private:
        const QString m_name;
        BaseHandler * m_handler;
        bool m_closed;
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
        explicit RootHandler(QTextDocument &document, const QString &name);
        virtual ~RootHandler();
    protected:
        virtual BaseHandler * NewTag(const QString & name, const QXmlAttributes &attributes);
    private:
        QTextDocument &m_document;
        Fb2TextCursor m_cursor1;
        bool m_empty;
    };

    class DescrHandler : public BaseHandler
    {
    public:
        explicit DescrHandler(const QString &name) : BaseHandler(name) {}
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
    };

    class TextHandler : public BaseHandler
    {
    public:
        explicit TextHandler(Fb2TextCursor &cursor, const QString &name);
        explicit TextHandler(TextHandler &parent, const QString &name);
    protected:
        virtual void EndTag(const QString &name);
    protected:
        Fb2TextCursor & cursor() { return m_cursor; }
        Fb2TextCursor & m_cursor;
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
        explicit BodyHandler(Fb2TextCursor &cursor, const QString &name);
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
    private:
        bool m_feed;
    };

    class ImageHandler : public TextHandler
    {
    public:
        explicit ImageHandler(TextHandler &parent, const QString &name, const QXmlAttributes &attributes);
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
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
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
        virtual void EndTag(const QString &name);
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
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
        virtual void EndTag(const QString &name);
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
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
        virtual void EndTag(const QString &name);
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
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
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
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
        virtual void TxtTag(const QString &text);
    };

    class BinaryHandler : public BaseHandler
    {
    public:
        explicit BinaryHandler(QTextDocument &document, const QString &name, const QXmlAttributes &attributes);
    protected:
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    private:
        QTextDocument & m_document;
        QString m_file;
        QString m_text;
    };

public:
    QTextDocument & document() { return m_document; }

private:
    QTextDocument & m_document;
    RootHandler * m_handler;
    QString m_error;
};

#undef FB2_BEGIN_KEYLIST

#undef FB2_END_KEYLIST

#endif // FB2READ_H
