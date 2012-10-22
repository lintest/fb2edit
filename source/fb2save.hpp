#ifndef FB2SAVE_H
#define FB2SAVE_H

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

class FbTextEdit;

class FbSaveDialog : public QFileDialog
{
    Q_OBJECT

public:
    explicit FbSaveDialog(QWidget *parent, Qt::WindowFlags f);

    explicit FbSaveDialog(QWidget *parent = 0,
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

class FbHtmlHandler : public QObject, public FbXmlHandler
{
    Q_OBJECT

public:
    explicit FbHtmlHandler() {}

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

class FbSaveWriter : public QXmlStreamWriter
{
public:
    explicit FbSaveWriter(FbTextEdit &view, QByteArray *array);
    explicit FbSaveWriter(FbTextEdit &view, QIODevice *device);
    explicit FbSaveWriter(FbTextEdit &view, QString *string);
    FbTextEdit & view() { return m_view; }
    QString getFileName(const QString &src);
    void writeStartElement(const QString &name, int level);
    void writeEndElement(int level);
    void writeComment(const QString &ch);
    void writeLineEnd();
    void writeFiles();
    void writeStyle();
private:
    QByteArray downloadFile(const QUrl &url);
    void writeContentType(const QString &name, QByteArray &data);
private:
    FbTextEdit &m_view;
    QStringList m_names;
    QString m_style;
};

class FbSaveHandler : public FbHtmlHandler
{
public:
    explicit FbSaveHandler(FbSaveWriter &writer);
    virtual bool comment(const QString& ch);
    bool save();

private:
    class TextHandler : public NodeHandler
    {
       FB2_BEGIN_KEYLIST
            Origin,
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
        explicit TextHandler(FbSaveWriter &writer, const QString &name, const QXmlAttributes &atts, const QString &tag);
        explicit TextHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag);
        const QString & tag() { return m_tag; }
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
        virtual void TxtTag(const QString &text);
        virtual void EndTag(const QString &name);
    protected:
        virtual void writeAtts(const QXmlAttributes &atts);
        virtual int nextLevel() const;
    protected:
        FbSaveWriter &m_writer;
        const QString m_tag;
        const int m_level;
    private:
        bool m_hasChild;
    };

    class RootHandler : public NodeHandler
    {
    public:
        explicit RootHandler(FbSaveWriter &writer, const QString &name);
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &atts);
    protected:
        FbSaveWriter &m_writer;
    };

    class BodyHandler : public TextHandler
    {
    public:
        explicit BodyHandler(FbSaveWriter &writer, const QString &name, const QXmlAttributes &atts);
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
    protected:
        virtual void writeAtts(const QXmlAttributes &atts);
    };

    class ImageHandler : public TextHandler
    {
    public:
        explicit ImageHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts);
    protected:
        virtual void writeAtts(const QXmlAttributes &atts);
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
    FbSaveWriter & m_writer;
};

#endif // FB2SAVE_H
