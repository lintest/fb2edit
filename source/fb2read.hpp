#ifndef FB2READ_H
#define FB2READ_H

#include "fb2xml.h"

#include <QByteArray>
#include <QMutex>
#include <QThread>
#include <QXmlDefaultHandler>

class Fb2ReadThread : public QThread
{
    Q_OBJECT
public:
    Fb2ReadThread(QObject *parent, const QString &filename, const QString &xml = QString());
    ~Fb2ReadThread();
    QString * data() { return &m_html; }

signals:
    void html(QString name, QString html);

public slots:
    void stop();

protected:
    void run();

private:
    bool parse();

private:
    const QString m_filename;
    const QString m_xml;
    QString m_html;
    bool m_abort;
    QMutex mutex;
};

class Fb2ReadHandler : public Fb2XmlHandler
{
public:
    explicit Fb2ReadHandler(Fb2ReadThread &thread, QXmlStreamWriter &writer);
    virtual ~Fb2ReadHandler();
    virtual bool comment(const QString& ch);
    Fb2ReadThread & thread() { return m_thread; }
    QXmlStreamWriter & writer() { return m_writer; }

private:
    class BaseHandler : public NodeHandler
    {
    public:
        explicit BaseHandler(Fb2ReadHandler &owner, const QString &name)
            : NodeHandler(name), m_owner(owner) {}
    protected:
        QXmlStreamWriter & writer() { return m_owner.writer(); }
    protected:
        Fb2ReadHandler &m_owner;
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
        explicit RootHandler(Fb2ReadHandler &owner, const QString &name);
    protected:
        virtual NodeHandler * NewTag(const QString & name, const QXmlAttributes &atts);
        virtual void EndTag(const QString &name);
    };

    class StyleHandler : public BaseHandler
    {
    public:
        explicit StyleHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts);
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
        explicit HeadHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts);
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
        explicit DescrHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts)
            : HeadHandler(owner, name, atts) {}
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
    };

    class TitleHandler : public HeadHandler
    {
    public:
        explicit TitleHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts)
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
        explicit TextHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts, const QString &tag, const QString &style = QString());
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
        explicit ImageHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts);
    };

    class BinaryHandler : public BaseHandler
    {
    public:
        explicit BinaryHandler(Fb2ReadHandler &owner, const QString &name, const QXmlAttributes &atts);
    protected:
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    private:
        QString m_file;
        QString m_type;
        QString m_text;
    };

protected:
    virtual NodeHandler * CreateRoot(const QString &name, const QXmlAttributes &atts);

private:
    void addFile(const QString &name, const QString &type, const QByteArray &data);
    QString getFile(const QString &name);

private:
    typedef QHash<QString, QString> StringHash;
    Fb2ReadThread &m_thread;
    QXmlStreamWriter &m_writer;
    StringHash m_hash;
};

#endif // FB2READ_H
