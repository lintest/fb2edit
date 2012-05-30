#ifndef Fb2Save_H
#define Fb2Save_H

#include "fb2xml.h"
#include "fb2temp.hpp"

#include <QByteArray>
#include <QStringList>
#include <QThread>
#include <QXmlDefaultHandler>
#include <QXmlStreamWriter>

#include "fb2temp.hpp"

class Fb2WebView;

class Fb2SaveWriter : public QXmlStreamWriter
{
public:
    explicit Fb2SaveWriter(Fb2WebView &view, QByteArray *array, QList<int> *folds = 0);
    explicit Fb2SaveWriter(Fb2WebView &view, QIODevice *device);
    explicit Fb2SaveWriter(Fb2WebView &view, QString *string);
    virtual ~Fb2SaveWriter();
    QString getFileName(const QString &src);
    void writeStartElement(const QString &name, int level);
    void writeEndElement(int level);
    void writeLineEnd();
    void writeFiles();
private:
    void Init();
    QByteArray downloadFile(const QUrl &url);
    QString getFileData(const QString &name);
    QString newFileName(const QString &path);
private:
    QList<int> *m_folds;
    Fb2WebView &m_view;
    Fb2TemporaryList m_files;
    QStringList m_names;
    int m_line;
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
        explicit BodyHandler(Fb2SaveWriter &writer, const QString &name, const QXmlAttributes &atts, const QString &tag);
        explicit BodyHandler(BodyHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag);
        const QString & tag() { return m_tag; }
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    protected:
        void Init(const QXmlAttributes &atts);
        virtual int nextLevel() const;
    protected:
        Fb2SaveWriter &m_writer;
        const QString m_tag;
        const int m_level;
    private:
        bool m_hasChild;
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
        virtual int nextLevel() const { return 0; }
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
