#ifndef FB2READ_H
#define FB2READ_H

#include "fb2xml.h"

#include <QByteArray>
#include <QMutex>
#include <QThread>
#include <QXmlDefaultHandler>

class FbTextPage;

class FbReadThread : public QThread
{
    Q_OBJECT
public:
    FbReadThread(QObject *parent, const QString &filename, const QString &xml = QString());
    ~FbReadThread();

public:
    void setPage(FbTextPage *page) { m_page = page; }
    void setTemp(QObject *temp) { m_temp = temp; }
    FbTextPage * page() const { return m_page; }
    QObject * temp() const { return m_temp; }
    QString * data() { return &m_html; }

signals:
    void html(QString name);

public slots:
    void stop();

protected:
    void run();

private:
    bool parse();

private:
    FbTextPage *m_page;
    QObject *m_temp;
    const QString m_filename;
    const QString m_xml;
    QString m_html;
    bool m_abort;
    QMutex mutex;
};

class FbReadHandler : public FbXmlHandler
{
public:
    explicit FbReadHandler(FbReadThread &thread, QXmlStreamWriter &writer);
    virtual ~FbReadHandler();
    virtual bool comment(const QString& ch);
    FbReadThread & thread() { return m_thread; }
    QXmlStreamWriter & writer() { return m_writer; }

private:
    class BaseHandler : public NodeHandler
    {
    public:
        explicit BaseHandler(FbReadHandler &owner, const QString &name)
            : NodeHandler(name), m_owner(owner) {}
    protected:
        QXmlStreamWriter & writer() { return m_owner.writer(); }
        void writeAttributes(const QXmlAttributes &atts);
    protected:
        FbReadHandler &m_owner;
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
        explicit RootHandler(FbReadHandler &owner, const QString &name);
    protected:
        virtual NodeHandler * NewTag(const QString & name, const QXmlAttributes &atts);
        virtual void EndTag(const QString &name);
    };

    class StyleHandler : public BaseHandler
    {
    public:
        explicit StyleHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts);
    protected:
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    private:
        bool m_empty;
    };

    class HeadHandler : public BaseHandler
    {
        FB2_BEGIN_KEYLIST
            Image,
        FB2_END_KEYLIST
    public:
        explicit HeadHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts);
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    private:
        bool m_empty;
    };

    class DescrHandler : public HeadHandler
    {
        FB2_BEGIN_KEYLIST
            Title,
            Document,
            Publish,
            Custom,
        FB2_END_KEYLIST
    public:
        explicit DescrHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts)
            : HeadHandler(owner, name, atts) {}
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
    };

    class TitleHandler : public HeadHandler
    {
    public:
        explicit TitleHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts)
            : HeadHandler(owner, name, atts) {}
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
    };

    class TextHandler : public BaseHandler
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
        explicit TextHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts, const QString &tag, const QString &style = QString());
        explicit TextHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag, const QString &style = QString());
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    protected:
        void Init(const QXmlAttributes &atts);
        bool isNotes() const;
    protected:
        TextHandler *m_parent;
        QString m_tag;
        QString m_style;
    };

    class SpanHandler : public TextHandler
    {
    public:
        explicit SpanHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts);
    };

    class AnchorHandler : public TextHandler
    {
    public:
        explicit AnchorHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts);
    };

    class ImageHandler : public TextHandler
    {
    public:
        explicit ImageHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts);
    };

    class BinaryHandler : public BaseHandler
    {
    public:
        explicit BinaryHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts);
    protected:
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    private:
        QString m_file;
        QString m_text;
    };

protected:
    virtual NodeHandler * CreateRoot(const QString &name, const QXmlAttributes &atts);

private:
    void addFile(const QString &name, const QByteArray &data);
    void writeScript(const QString &src);

private:
    typedef QHash<QString, QString> StringHash;
    FbReadThread &m_thread;
    QXmlStreamWriter &m_writer;
    QObject *m_temp;
    StringHash m_hash;
};

#endif // FB2READ_H
