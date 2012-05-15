#ifndef Fb2Save_H
#define Fb2Save_H

#include "fb2xml.h"

#include <QByteArray>
#include <QMutex>
#include <QThread>
#include <QXmlDefaultHandler>
#include <QXmlStreamWriter>

class Fb2SaveThread : public QThread
{
    Q_OBJECT

public:
    Fb2SaveThread(QObject *parent, const QString &filename);
    ~Fb2SaveThread();
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

class Fb2SaveWriter : public QXmlStreamWriter
{
public:
    explicit Fb2SaveWriter(Fb2SaveThread &thread);
    QString addFile(const QString &name, const QByteArray &data);
    QString getFile(const QString &name);
    QString newId();
private:
    typedef QHash<QString, QString> StringHash;
    Fb2SaveThread &m_thread;
    StringHash m_hash;
    int m_id;
};

class Fb2SaveHandler : public Fb2XmlHandler
{
public:
    explicit Fb2SaveHandler(Fb2SaveThread &thread);

private:
    class BodyHandler : public NodeHandler
    {
        FB2_BEGIN_KEYLIST
            Section,
            Anchor,
            Table,
            Image,
            Parag,
            Strong,
            Emphas,
            Strike,
            Sub,
            Sup,
            Code,
       FB2_END_KEYLIST
    public:
        explicit BodyHandler(Fb2SaveWriter &writer, const QString &name, const QXmlAttributes &atts, const QString &tag, const QString &style = QString());
        explicit BodyHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag, const QString &style = QString());
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    protected:
        void Init(const QXmlAttributes &atts);
    protected:
        Fb2SaveWriter &m_writer;
        QString m_tag;
        QString m_style;
    };

    class AnchorHandler : public BodyHandler
    {
    public:
        explicit AnchorHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &atts);
    };

    class ImageHandler : public BodyHandler
    {
    public:
        explicit ImageHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &atts);
    };

protected:
    virtual NodeHandler * CreateRoot(const QString &name, const QXmlAttributes &atts);

private:
    Fb2SaveWriter m_writer;
};

#endif // Fb2Save_H
