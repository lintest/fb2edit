#ifndef FB2HTML_H
#define FB2HTML_H

#include <QUndoCommand>
#include <QWebElement>

class FbTextPage;

class FbTextElement;

typedef QList<FbTextElement> FbElementList;

class FbTextElement : public QWebElement
{
public:
    FbTextElement() {}
    FbTextElement(const QWebElement &x) : QWebElement(x) {}
    FbTextElement &operator=(const QWebElement &x) { QWebElement::operator=(x); return *this; }
    void getChildren(FbElementList &list);
    QString location();
    bool hasTitle() const;
    bool isBody() const;
    bool isSection() const;
    bool isTitle() const;

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

#endif // FB2HTML_H
