#ifndef Fb2Save_H
#define Fb2Save_H

#include "fb2xml.h"
#include "fb2temp.hpp"

#include <QByteArray>
#include <QFileDialog>
#include <QStringList>
#include <QXmlStreamWriter>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
QT_END_NAMESPACE

#include "fb2temp.hpp"

class Fb2WebView;

class Fb2SaveDialog : public QFileDialog
{
    Q_OBJECT

public:
    explicit Fb2SaveDialog(QWidget *parent, Qt::WindowFlags f);

    explicit Fb2SaveDialog(QWidget *parent = 0,
                         const QString &caption = QString(),
                         const QString &directory = QString(),
                         const QString &filter = QString());

    QString fileName() const;

    QString codec() const;

private:
    void init();

private:
    QComboBox * combo;
    QLabel * label;
};

class Fb2HtmlHandler : public QObject, public Fb2XmlHandler
{
    Q_OBJECT

public:
    explicit Fb2HtmlHandler() {}

public slots:
    void onAttr(const QString &name, const QString &value);
    void onNew(const QString &name);
    void onEnd(const QString &name);
    void onTxt(const QString &text);
    void onCom(const QString &text);

private:
    static QString local(const QString &name);

private:
    QXmlAttributes m_atts;
};

class Fb2SaveWriter : public QXmlStreamWriter
{
public:
    explicit Fb2SaveWriter(Fb2WebView &view, QByteArray *array);
    explicit Fb2SaveWriter(Fb2WebView &view, QIODevice *device);
    explicit Fb2SaveWriter(Fb2WebView &view, QString *string);
    Fb2WebView & view() { return m_view; }
    QString getFileName(const QString &src);
    void writeStartElement(const QString &name, int level);
    void writeEndElement(int level);
    void writeComment(const QString &ch);
    void writeLineEnd();
    void writeFiles();
private:
    QByteArray downloadFile(const QUrl &url);
private:
    Fb2WebView &m_view;
    QStringList m_names;
};

class Fb2SaveHandler : public Fb2HtmlHandler
{
public:
    explicit Fb2SaveHandler(Fb2SaveWriter &writer);
    virtual bool comment(const QString& ch);
    bool save();

private:
    class TextHandler : public NodeHandler
    {
        FB2_BEGIN_KEYLIST
            Section,
            Anchor,
            Table,
            Image,
            Span,
            Parag,
            Strong,
            Emphas,
            Strike,
            Sub,
            Sup,
            Code,
       FB2_END_KEYLIST
    public:
        explicit TextHandler(Fb2SaveWriter &writer, const QString &name, const QXmlAttributes &atts, const QString &tag);
        explicit TextHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag);
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

    class RootHandler : public NodeHandler
    {
    public:
        explicit RootHandler(Fb2SaveWriter &writer, const QString &name);
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
    protected:
        Fb2SaveWriter &m_writer;
    };

    class BodyHandler : public TextHandler
    {
    public:
        explicit BodyHandler(Fb2SaveWriter &writer, const QString &name, const QXmlAttributes &atts);
    protected:
        virtual void EndTag(const QString &name);
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
        explicit ImageHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts);
    protected:
        virtual void EndTag(const QString &name) { Q_UNUSED(name); }
    };

    class ParagHandler : public TextHandler
    {
    public:
        explicit ParagHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts);
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
    Fb2SaveWriter & m_writer;
};

#endif // Fb2Save_H