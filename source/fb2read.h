#ifndef FB2READ_H
#define FB2READ_H

#include "fb2xml.h"

#include <QByteArray>
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

class Fb2ReadWriter : public QXmlStreamWriter
{
public:
    explicit Fb2ReadWriter(Fb2ReadThread &thread);
    QString addFile(const QString &name, const QByteArray &data);
    QString getFile(const QString &name);
    QString newId();
private:
    typedef QHash<QString, QString> StringHash;
    Fb2ReadThread &m_thread;
    StringHash m_hash;
    int m_id;
};

class Fb2ReadHandler : public Fb2XmlHandler
{
public:
    explicit Fb2ReadHandler(Fb2ReadThread &thread);

private:
    class BaseHandler : public NodeHandler
    {
    public:
        explicit BaseHandler(Fb2ReadWriter &writer, const QString &name)
            : NodeHandler(name), m_writer(writer) {}
    protected:
        Fb2ReadWriter &m_writer;
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
        explicit RootHandler(Fb2ReadWriter &writer, const QString &name);
    protected:
        virtual NodeHandler * NewTag(const QString & name, const QXmlAttributes &attributes);
        virtual void EndTag(const QString &name);
    };

    class HeadHandler : public BaseHandler
    {
    public:
        explicit HeadHandler(Fb2ReadWriter &writer, const QString &name, bool hide = false);
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
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
        explicit DescrHandler(Fb2ReadWriter &writer, const QString &name);
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
    };

    class TitleHandler : public HeadHandler
    {
    public:
        explicit TitleHandler(Fb2ReadWriter &writer, const QString &name);
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
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
        explicit BodyHandler(Fb2ReadWriter &writer, const QString &name, const QXmlAttributes &attributes, const QString &tag, const QString &style = QString());
        explicit BodyHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &attributes, const QString &tag, const QString &style = QString());
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &attributes);
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    protected:
        void Init(const QXmlAttributes &attributes);
        bool isNotes() const;
    protected:
        BodyHandler *m_parent;
        QString m_tag;
        QString m_style;
    };

    class AnchorHandler : public BodyHandler
    {
    public:
        explicit AnchorHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &attributes);
    };

    class ImageHandler : public BodyHandler
    {
    public:
        explicit ImageHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &attributes);
    };

    class BinaryHandler : public BaseHandler
    {
    public:
        explicit BinaryHandler(Fb2ReadWriter &writer, const QString &name, const QXmlAttributes &attributes);
    protected:
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    private:
        QString m_file;
        QString m_text;
    };

protected:
    virtual NodeHandler * CreateRoot(const QString &name);

private:
    Fb2ReadWriter m_writer;
};

#endif // FB2READ_H
