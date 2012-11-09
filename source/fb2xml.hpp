#ifndef FB2XML_H
#define FB2XML_H

#include <QHash>
#include <QXmlDefaultHandler>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "fb2logs.hpp"

#define FB2_BEGIN_KEYLIST private: enum Keyword {

#define FB2_END_KEYLIST None }; \
class KeywordHash : public QHash<QString, Keyword> { public: KeywordHash(); }; \
static Keyword toKeyword(const QString &name); private:

#define FB2_BEGIN_KEYHASH(x) \
x::Keyword x::toKeyword(const QString &name) \
{                                                                    \
    static const KeywordHash map;                                    \
    KeywordHash::const_iterator i = map.find(name);                  \
    return i == map.end() ? None : i.value();                        \
}                                                                    \
x::KeywordHash::KeywordHash() {

#define FB2_END_KEYHASH }

#define FB2_KEY(key,str) insert(str,key);

class FbXmlHandler : public QObject, public QXmlDefaultHandler
{
    Q_OBJECT

public:
    explicit FbXmlHandler();
    virtual ~FbXmlHandler();
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
    bool characters(const QString &str);
    bool error(const QXmlParseException& exception);
    bool warning(const QXmlParseException& exception);
    bool fatalError(const QXmlParseException &exception);
    QString errorString() const;

signals:
    void warning(int row, int col, const QString &msg);
    void error(int row, int col, const QString &msg);
    void fatal(int row, int col, const QString &msg);

protected:
    class NodeHandler
    {
    public:
        static QString Value(const QXmlAttributes &attributes, const QString &name);
        explicit NodeHandler(const QString &name)
            : m_name(name), m_handler(0), m_closed(false) {}
        virtual ~NodeHandler()
            { if (m_handler) delete m_handler; }
        bool doStart(const QString &name, const QXmlAttributes &attributes);
        bool doText(const QString &text);
        bool doEnd(const QString &name, bool & found);
    protected:
        virtual NodeHandler * NewTag(const QString &name, const QXmlAttributes &attributes)
            { Q_UNUSED(name); Q_UNUSED(attributes); return NULL; }
        virtual void TxtTag(const QString &text)
            { Q_UNUSED(text); }
        virtual void EndTag(const QString &name)
            { Q_UNUSED(name); }
        const QString & Name() const
            { return m_name; }
    private:
        const QString m_name;
        NodeHandler * m_handler;
        bool m_closed;
    };

protected:
    virtual NodeHandler * CreateRoot(const QString &name, const QXmlAttributes &attributes) = 0;
    static bool isWhiteSpace(const QString &str);

protected:
    NodeHandler * m_handler;
    QString m_error;
};

#endif // FB2XML_H
