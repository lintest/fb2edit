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
    protected:
        FbReadHandler &m_owner;
    };

    class RootHandler : public BaseHandler
    {
    public:
        explicit RootHandler(FbReadHandler &owner, const QString &name);
    protected:
        virtual NodeHandler * NewTag(const QString & name, const QXmlAttributes &atts);
        virtual void EndTag(const QString &name);
    };

    class TextHandler : public BaseHandler
    {
        FB2_BEGIN_KEYLIST
            Origin,
            Anchor,
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
        explicit TextHandler(FbReadHandler &owner, const QString &name, const QXmlAttributes &atts, const QString &tag);
        explicit TextHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag);
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    protected:
        void Init(const QString &name, const QXmlAttributes &atts);
        bool isNotes() const;
    protected:
        TextHandler *m_parent;
        QString m_tag;
        QString m_style;
        bool m_empty;
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
