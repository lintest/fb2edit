#ifndef FB2HTML_H
#define FB2HTML_H

#include <QUndoCommand>
#include <QWebElement>

class FbTextPage;

class FbTextElement;

typedef QList<FbTextElement> FbElementList;

class FbTextScheme
{
public:
    explicit FbTextScheme();

    class Type
    {
    public:
        Type(const QString &name = QString(), int min=0, int max=1): m_name(name), m_min(min), m_max(max) {}
        Type(const Type &t): m_name(t.m_name), m_min(t.m_min), m_max(t.m_max) {}
        const QString & name() const { return m_name; }
        int min() { return m_min; }
        int max() { return m_max; }
    private:
        const QString m_name;
        const int m_min;
        const int m_max;
    };

    typedef QList<Type> TypeList;

    typedef QMap<QString, TypeList> TypeMap;

private:
    TypeMap m_types;
};

class FbTextElement : public QWebElement
{
public:
    FbTextElement() {}
    FbTextElement(const QWebElement &x) : QWebElement(x) {}
    FbTextElement &operator=(const QWebElement &x) { QWebElement::operator=(x); return *this; }
    void getChildren(FbElementList &list);
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
