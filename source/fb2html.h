#ifndef FB2HTML_H
#define FB2HTML_H

#include <QUndoCommand>
#include <QWebElement>

class Fb2TextPage;

class Fb2TextElement;

typedef QList<Fb2TextElement> Fb2ElementList;

class Fb2TextElement : public QWebElement
{
public:
    Fb2TextElement() {}
    Fb2TextElement(const QWebElement &x) : QWebElement(x) {}
    Fb2TextElement &operator=(const QWebElement &x) { QWebElement::operator=(x); return *this; }
    void getChildren(Fb2ElementList &list);
    QString location();
    bool hasTitle() const;
    bool isBody() const;
    bool isSection() const;
    bool isTitle() const;

public:
    Fb2TextElement findFirst(const QString &selectorQuery) const { return QWebElement::findFirst(selectorQuery); }
    Fb2TextElement parent() const { return QWebElement::parent(); }
    Fb2TextElement firstChild() const { return QWebElement::firstChild(); }
    Fb2TextElement lastChild() const { return QWebElement::lastChild(); }
    Fb2TextElement nextSibling() const { return QWebElement::nextSibling(); }
    Fb2TextElement previousSibling() const { return QWebElement::previousSibling(); }
    Fb2TextElement document() const { return QWebElement::document(); }

public:
    void select();
};

class Fb2InsertCmd : public QUndoCommand
{
public:
    explicit Fb2InsertCmd(const Fb2TextElement &element);
    virtual void undo();
    virtual void redo();
private:
    Fb2TextElement m_parent;
    Fb2TextElement m_element;
    bool m_inner;
};

class Fb2DeleteCmd : public QUndoCommand
{
public:
    explicit Fb2DeleteCmd(const Fb2TextElement &element);
    virtual void undo();
    virtual void redo();
private:
    Fb2TextElement m_element;
    Fb2TextElement m_parent;
    bool m_inner;
};

class Fb2MoveUpCmd : public QUndoCommand
{
public:
    explicit Fb2MoveUpCmd(const Fb2TextElement &element);
    virtual void undo();
    virtual void redo();
private:
    Fb2TextElement m_element;
};

#endif // FB2HTML_H
