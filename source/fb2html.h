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

class Fb2UndoCommand : public QUndoCommand
{
public:
    explicit Fb2UndoCommand(QUndoCommand *parent = 0) : QUndoCommand(parent) {}
protected:
    static QString div(const QString &style, const QString &text);
    static QString p(const QString &text = "<br/>");
};

class Fb2AddBodyCmd : public Fb2UndoCommand
{
public:
    explicit Fb2AddBodyCmd(Fb2TextPage &page, Fb2UndoCommand *parent = 0) : Fb2UndoCommand(parent), m_page(page) {}
    virtual void undo();
    virtual void redo();
private:
    Fb2TextPage & m_page;
    Fb2TextElement m_body;
};

class Fb2SectionCmd : public Fb2UndoCommand
{
public:
    explicit Fb2SectionCmd(Fb2TextPage &page, const Fb2TextElement &element, Fb2UndoCommand *parent = 0)
        : Fb2UndoCommand(parent), m_page(page), m_parent(element) {}
    virtual void undo();
    virtual void redo();
private:
    Fb2TextPage & m_page;
    Fb2TextElement m_parent;
    Fb2TextElement m_child;
};

class Fb2TitleCmd : public Fb2UndoCommand
{
public:
    explicit Fb2TitleCmd(Fb2TextPage &page, const Fb2TextElement &element, Fb2UndoCommand *parent = 0)
        : Fb2UndoCommand(parent), m_page(page), m_section(element) {}
    virtual void undo();
    virtual void redo();
private:
    Fb2TextPage & m_page;
    Fb2TextElement m_section;
    Fb2TextElement m_title;
};

class Fb2SubtitleCmd : public Fb2UndoCommand
{
public:
    explicit Fb2SubtitleCmd(Fb2TextPage &page, const QString &location, Fb2UndoCommand *parent = 0)
        : Fb2UndoCommand(parent), m_page(page), m_location(location) {}
    virtual void undo();
    virtual void redo();
private:
    Fb2TextElement m_element;
    Fb2TextPage & m_page;
    QString m_location;
    QString m_position;
};

class Fb2MoveUpCmd : public Fb2UndoCommand
{
public:
    explicit Fb2MoveUpCmd(Fb2TextPage &page, const Fb2TextElement &element, Fb2UndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
private:
    Fb2TextPage & m_page;
    Fb2TextElement m_element;
};

class Fb2DeleteCmd : public Fb2UndoCommand
{
public:
    explicit Fb2DeleteCmd(Fb2TextPage &page, const Fb2TextElement &element, Fb2UndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
private:
    Fb2TextElement m_element;
    Fb2TextElement m_parent;
    Fb2TextPage & m_page;
    QString m_position;
    bool m_inner;
};

#endif // FB2HTML_H
