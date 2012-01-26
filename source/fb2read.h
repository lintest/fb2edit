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
    Fb2Handler(QTextDocument & document);
    virtual ~Fb2Handler();
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &exception);
    QString errorString() const;

private:
    class ContentHandler
    {
    public:
        ContentHandler(Fb2Handler & owner) : m_owner(owner), m_cursor(owner.getCursor()), m_parent(NULL) {}
        ContentHandler(ContentHandler * parent) : m_owner(parent->getOwner()), m_cursor(m_owner.getCursor()), m_parent(parent) {}
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes) = 0;
        virtual bool doText(const QString &text) = 0;
        virtual bool doEnd(const QString &name, bool & exit) = 0;
        Fb2Handler & getOwner() { return m_owner; }
        ContentHandler * getParent() { return m_parent; }
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
        virtual bool doStart(const QString & name, const QXmlAttributes &attributes);
        virtual bool doText(const QString & text) { Q_UNUSED(text); return true; }
        virtual bool doEnd(const QString & name, bool & exit) { Q_UNUSED(name); exit = true; return true; }
    private:
        enum Keyword {
            None = 0,
            Body,
            Descr,
            Binary,
        };
        class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); };
        static Keyword toKeyword(const QString & name);
    };

    class DescrHandler : public ContentHandler
    {
    public:
        DescrHandler(ContentHandler * parent) : ContentHandler(parent) {}
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doText(const QString &text);
        virtual bool doEnd(const QString &name, bool & exit);
    };

    class BodyHandler : public ContentHandler
    {
    public:
        BodyHandler(ContentHandler * parent) : ContentHandler(parent) {}
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doText(const QString &text);
        virtual bool doEnd(const QString &name, bool & exit);
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
        static Keyword toKeyword(const QString & name);
    private:
        QList<QTextFrame*> m_frames;
    };

    class BinaryHandler : public ContentHandler
    {
    public:
        BinaryHandler(ContentHandler * parent, const QXmlAttributes &attributes);
        virtual bool doStart(const QString &name, const QXmlAttributes &attributes);
        virtual bool doText(const QString &text);
        virtual bool doEnd(const QString &name, bool & exit);
    private:
        QString m_name;
        QString m_text;
    };

public:
    bool setHandler(ContentHandler * handler) { m_handler = handler; return handler; }
    QTextDocument & getDocument() { return m_document; }
    QTextCursor & getCursor() { return m_cursor; }

private:
    static ContentHandler * doDelete(ContentHandler * handler);

private:
    QTextDocument & m_document;
    QTextCursor m_cursor;
    QString m_name;
    QString m_text;
    QString m_error;
    QStringList m_tags;
    ContentHandler * m_handler;
};

#endif // FB2READ_H
