#ifndef FB2HTML_H
#define FB2HTML_H

#include <QUndoCommand>
#include <QWebElement>

class FbTextPage;

class FbTextElement;

typedef QList<FbTextElement> FbElementList;

class FbTextElement : public QWebElement
{
private:
    class Type
    {
    public:
        Type(const QString &name = QString(), int min=0, int max=1): m_name(name), m_min(min), m_max(max) {}
        Type(const Type &t): m_name(t.m_name), m_min(t.m_min), m_max(t.m_max) {}
        const QString & name() const { return m_name; }
        int min() const { return m_min; }
        int max() const { return m_max; }
    private:
        QString m_name;
        int m_min;
        int m_max;
    };

    typedef QList<Type> TypeList;

    typedef QMap<QString, TypeList> TypeMap;

    class Scheme
    {
    public:
        explicit Scheme();
        const TypeList * operator[](const QString &name) const;
    private:
        TypeMap m_types;
    };

    class Sublist
    {
    public:
        Sublist(const TypeList &list, const QString &name);
        operator bool() const;
        bool operator !() const;
        bool operator <(const QWebElement &element) const;
        bool operator >=(const QWebElement &element) const;
        bool operator !=(const QWebElement &element) const;
    private:
        const TypeList &m_list;
        TypeList::const_iterator m_pos;
    };

public:
    FbTextElement() {}
    FbTextElement(const QWebElement &x) : QWebElement(x) {}
    FbTextElement &operator=(const QWebElement &x) { QWebElement::operator=(x); return *this; }
    FbTextElement insertInside(const QString &style, const QString &html);
    void getChildren(FbElementList &list);
    bool hasSubtype(const QString &style) const;
    bool hasScheme() const;
    QString location();

public:
    bool hasChild(const QString &style) const;
    bool isDiv(const QString &style) const;
    bool isBody() const;
    bool isSection() const;
    bool isTitle() const;
    bool isStanza() const;
    bool hasTitle() const;

public:
    FbTextElement findFirst(const QString &selectorQuery) const { return QWebElement::findFirst(selectorQuery); }
    FbTextElement parent() const { return QWebElement::parent(); }
    FbTextElement firstChild() const { return QWebElement::firstChild(); }
    FbTextElement lastChild() const { return QWebElement::lastChild(); }
    FbTextElement nextSibling() const { return QWebElement::nextSibling(); }
    FbTextElement previousSibling() const { return QWebElement::previousSibling(); }
    FbTextElement document() const { return QWebElement::document(); }

public:
    void select();

private:
    const TypeList *subtypes() const;
    TypeList::const_iterator subtype(const TypeList &list, const QString &style);
};

class FbInsertCmd : public QUndoCommand
{
public:
    explicit FbInsertCmd(const FbTextElement &element);
    virtual void undo();
    virtual void redo();
private:
    FbTextElement m_element;
    FbTextElement m_parent;
    bool m_inner;
};

class FbReplaceCmd : public QUndoCommand
{
public:
    explicit FbReplaceCmd(const FbTextElement &original, const FbTextElement &element);
    virtual void undo();
    virtual void redo();
private:
    FbTextElement m_original;
    FbTextElement m_element;
    bool m_update;
};

class FbDeleteCmd : public QUndoCommand
{
public:
    explicit FbDeleteCmd(const FbTextElement &element);
    virtual void undo();
    virtual void redo();
private:
    FbTextElement m_element;
    FbTextElement m_parent;
    bool m_inner;
};

class FbMoveUpCmd : public QUndoCommand
{
public:
    explicit FbMoveUpCmd(const FbTextElement &element);
    virtual void undo();
    virtual void redo();
private:
    FbTextElement m_element;
};

class FbMoveLeftCmd : public QUndoCommand
{
public:
    explicit FbMoveLeftCmd(const FbTextElement &element);
    virtual void undo();
    virtual void redo();
private:
    FbTextElement m_element;
    FbTextElement m_subling;
    FbTextElement m_parent;
};

class FbMoveRightCmd : public QUndoCommand
{
public:
    explicit FbMoveRightCmd(const FbTextElement &element);
    virtual void undo();
    virtual void redo();
private:
    FbTextElement m_element;
    FbTextElement m_subling;
};

#endif // FB2HTML_H
