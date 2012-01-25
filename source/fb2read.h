#ifndef FB2READ_H
#define FB2READ_H

#include <QXmlDefaultHandler>
#include <QTextCursor>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QTextEdit;
QT_END_NAMESPACE

class Fb2Handler : public QXmlDefaultHandler
{
public:
    Fb2Handler(QTextEdit * editor);
    virtual ~Fb2Handler();
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &exception);
    QString errorString() const;

private:
    enum DocSection {
        None = 0,
        Body,
        Descr,
        Binary,
    };

    enum DocKeyword {
        Empty = 0,
        Image,
        Paragraph,
        Section,
        Title,
    };

    class ContentHandler
    {
    public:
        ContentHandler(Fb2Handler & owner) : m_owner(owner), m_cursor(owner.GetCursor()), m_parent(NULL) {}
        ContentHandler(ContentHandler * parent) : m_owner(parent->GetOwner()), m_cursor(m_owner.GetCursor()), m_parent(parent) {}
        virtual bool DoStart(const QString &name, const QXmlAttributes &attributes) = 0;
        virtual bool DoText(const QString &text) = 0;
        virtual bool DoEnd(const QString &name, bool & exit) = 0;
        Fb2Handler & GetOwner() { return m_owner; }
        ContentHandler * GetParent() { return m_parent; }
    protected:
        Fb2Handler & m_owner;
        QTextCursor & m_cursor;
    private:
        ContentHandler * m_parent;
    };

    class RootHandler : public ContentHandler
    {
    public:
        RootHandler(Fb2Handler & owner) : ContentHandler(owner) {}
        virtual bool DoStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool DoText(const QString &text) {}
        virtual bool DoEnd(const QString &name, bool & exit) { exit = true; }
    private:
        enum Keyword {
            None = 0,
            Body,
            Descr,
            Binary,
        };
        class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); };
        static Keyword GetKeyword(const QString & name);
    };

    class DescrHandler : public ContentHandler
    {
    public:
        DescrHandler(ContentHandler * parent) : ContentHandler(parent) {}
        virtual bool DoStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool DoText(const QString &text);
        virtual bool DoEnd(const QString &name, bool & exit);
    };

    class BodyHandler : public ContentHandler
    {
    public:
        BodyHandler(ContentHandler * parent) : ContentHandler(parent) {}
        virtual bool DoStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool DoText(const QString &text);
        virtual bool DoEnd(const QString &name, bool & exit);
    private:
        enum Keyword {
            None = 0,
            Body,
            Image,
            Paragraph,
            Section,
            Title,
        };
        class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); };
        static Keyword GetKeyword(const QString & name);
    private:
        QList<QTextFrame*> m_frames;
    };

    class ImageHandler : public ContentHandler
    {
    public:
        ImageHandler(ContentHandler * parent, const QXmlAttributes &attributes);
        virtual bool DoStart(const QString &name, const QXmlAttributes &attributes) { return true; }
        virtual bool DoText(const QString &text) { m_text += text; return true; }
        virtual bool DoEnd(const QString &name, bool & exit) { exit = true; return true; }
    private:
        QString m_name;
        QString m_text;
    };

public:
    bool SetHandler(ContentHandler * handler) { m_handler = handler; return handler; }
    QTextCursor & GetCursor() { return m_cursor; }

private:
    static ContentHandler * Up(ContentHandler * handler);
    bool BodyNew(const QString &name, const QXmlAttributes &attributes);
    bool BodyEnd(const QString &name);

private:
    QTextEdit * m_editor;
    QTextCursor m_cursor;
    QString m_name;
    QString m_text;
    QString m_error;
    QStringList m_tags;
    DocSection m_section;
    ContentHandler * m_handler;
};

#endif // FB2READ_H
