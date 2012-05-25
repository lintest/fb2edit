#ifndef Fb2Save_H
#define Fb2Save_H

#include "fb2xml.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QThread>
#include <QXmlDefaultHandler>
#include <QXmlStreamWriter>

class Fb2WebView;

class Fb2SaveWriter : public QXmlStreamWriter
{
public:
    explicit Fb2SaveWriter(Fb2WebView &view, QByteArray *array, QList<int> *folds = 0);
    explicit Fb2SaveWriter(Fb2WebView &view, QIODevice *device);
    explicit Fb2SaveWriter(Fb2WebView &view, QString *string);
    virtual ~Fb2SaveWriter();
    QString getFile(const QString &path);
    QString getData(const QString &name);
    void writeFiles();
private:
    void Init();
private:
    QList<int> *m_folds;
    Fb2WebView &m_view;
    typedef QHash<QString, QString> StringHash;
    typedef QList<QString> StringList;
    StringHash m_files;
    StringList m_names;
};

class Fb2SaveHandler : public Fb2XmlHandler
{
public:
    explicit Fb2SaveHandler(Fb2WebView &view, QByteArray *array, QList<int> *folds);
    explicit Fb2SaveHandler(Fb2WebView &view, QIODevice *device);

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
        const QString & tag() { return m_tag; }
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

    class RootHandler : public BodyHandler
    {
    public:
        explicit RootHandler(Fb2SaveWriter &writer, const QString &name, const QXmlAttributes &atts);
    protected:
        virtual void EndTag(const QString &name);
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
    protected:
        virtual void EndTag(const QString &name) { Q_UNUSED(name); }
    };

    class ParagHandler : public BodyHandler
    {
    public:
        explicit ParagHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &atts);
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    private:
        void start();
    private:
        const QString m_parent;
        bool m_empty;
    };

protected:
    virtual NodeHandler * CreateRoot(const QString &name, const QXmlAttributes &atts);

private:
    Fb2SaveWriter m_writer;
};

#endif // Fb2Save_H
