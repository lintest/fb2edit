#ifndef FB2HTML_H
#define FB2HTML_H

#include <QUndoCommand>
#include <QWebElement>

class Fb2TextPage;

class Fb2TextElement : public QWebElement
{
public:
    Fb2TextElement() {}
    Fb2TextElement(const QWebElement &x) : QWebElement(x) {}
    Fb2TextElement &operator=(const QWebElement &x) { QWebElement::operator=(x); return *this; }
    QString location();
    bool isSection() const;
    bool isTitle() const;

public:
    void select();
};

class Fb2AddBodyCmd : public QUndoCommand
{
public:
    explicit Fb2AddBodyCmd(Fb2TextPage &page, QUndoCommand *parent = 0) : QUndoCommand(parent), m_page(page) {}
    virtual void undo();
    virtual void redo();
private:
    Fb2TextPage & m_page;
};

class Fb2SubtitleCmd : public QUndoCommand
{
public:
    explicit Fb2SubtitleCmd(Fb2TextPage &page, const QString &location, QUndoCommand *parent = 0)
        : QUndoCommand(parent), m_page(page), m_location(location) {}
    virtual void undo();
    virtual void redo();
private:
    Fb2TextElement m_element;
    Fb2TextPage & m_page;
    QString m_location;
    QString m_position;
};

class Fb2DeleteCmd : public QUndoCommand
{
public:
    explicit Fb2DeleteCmd(Fb2TextPage &page, Fb2TextElement &element, QUndoCommand *parent = 0);
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
