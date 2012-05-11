#ifndef FB2READ_H
#define FB2READ_H

#include <QByteArray>
#include <QHash>
#include <QMutex>
#include <QThread>
#include <QXmlDefaultHandler>
#include <QXmlStreamWriter>

class Fb2ReadThread : public QThread
{
    Q_OBJECT

public:
    Fb2ReadThread(QObject *parent, const QString &filename);
    ~Fb2ReadThread();
    void onFile(const QString &name, const QString &path);
    QString * data() { return &m_html; }

signals:
    void file(QString name, QString path);
    void html(QString name, QString html);

public slots:
    void stop();

protected:
    void run();

private:
    bool parse();

private:
    const QString m_filename;
    QString m_html;
    bool m_abort;
    QMutex mutex;
};

class Fb2HtmlWriter : public QXmlStreamWriter
{
public:
    explicit Fb2HtmlWriter(Fb2ReadThread &thread);
    QString addFile(const QString &name, const QByteArray &data);
    QString getFile(const QString &name);
    QString newId();
private:
    typedef QHash<QString, QString> StringHash;
    Fb2ReadThread &m_thread;
    StringHash m_hash;
    int m_id;
};

#define FB2_BEGIN_KEYLIST private: enum Keyword {

#define FB2_END_KEYLIST None }; \
class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); }; \
static Keyword toKeyword(const QString &name); private:

class Fb2Handler : public QXmlDefaultHandler
{
public:
    explicit Fb2Handler(Fb2ReadThread &thread);
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
        const QString & Name() const
            { return m_name; }
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
        explicit RootHandler(Fb2HtmlWriter &writer, const QString &name);
    protected:
        virtual BaseHandler * NewTag(const QString & name, const QXmlAttributes &attributes);
        virtual void EndTag(const QString &name);
    private:
        Fb2HtmlWriter &m_writer;
    };

    class DescrHandler : public BaseHandler
    {
        FB2_BEGIN_KEYLIST
            Title,
            Publish,
        FB2_END_KEYLIST
    public:
        explicit DescrHandler(Fb2HtmlWriter &writer, const QString &name) : BaseHandler(name), m_writer(writer) {}
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
    protected:
        Fb2HtmlWriter &m_writer;
    };

    class HeaderHandler : public BaseHandler
    {
        FB2_BEGIN_KEYLIST
            Author,
            Title,
            Sequence,
            Genre,
            Lang,
            Annot,
            Cover,
        FB2_END_KEYLIST
    public:
        explicit HeaderHandler(Fb2HtmlWriter &writer, const QString &name) : BaseHandler(name), m_writer(writer) {}
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
    protected:
        Fb2HtmlWriter &m_writer;
    };

    class BodyHandler : public BaseHandler
    {
        FB2_BEGIN_KEYLIST
            Section,
            Anchor,
            Table,
            Image,
            Parag,
            Style,
            Strong,
            Emphas,
            Strike,
            Sub,
            Sup,
            Code,
       FB2_END_KEYLIST
    public:
        explicit BodyHandler(Fb2HtmlWriter &writer, const QString &name, const QXmlAttributes &attributes, const QString &tag, const QString &style = QString());
        virtual void TxtTag(const QString &text);
    protected:
        virtual BaseHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
        virtual void EndTag(const QString &name);
    protected:
        Fb2HtmlWriter &m_writer;
        QString m_tag;
        QString m_style;
    };

    class AnchorHandler : public BodyHandler
    {
    public:
        explicit AnchorHandler(Fb2HtmlWriter &writer, const QString &name, const QXmlAttributes &attributes);
    };

    class ImageHandler : public BodyHandler
    {
    public:
        explicit ImageHandler(Fb2HtmlWriter &writer, const QString &name, const QXmlAttributes &attributes);
    };

    class BinaryHandler : public BaseHandler
    {
    public:
        explicit BinaryHandler(Fb2HtmlWriter &writer, const QString &name, const QXmlAttributes &attributes);
    protected:
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    private:
        Fb2HtmlWriter &m_writer;
        QString m_file;
        QString m_text;
    };

private:
    Fb2HtmlWriter m_writer;
    RootHandler * m_handler;
    QString m_error;
};

#undef FB2_BEGIN_KEYLIST

#undef FB2_END_KEYLIST

#endif // FB2READ_H
