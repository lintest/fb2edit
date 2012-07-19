#include "fb2html.h"
#include "fb2utils.h"

#include "fb2text.hpp"

//---------------------------------------------------------------------------
//  Fb2TextElement
//---------------------------------------------------------------------------

void Fb2TextElement::getChildren(Fb2ElementList &list)
{
    Fb2TextElement child = firstChild();
    while (!child.isNull()) {
        QString tag = child.tagName().toLower();
        if (tag == "div") {
            if (child.hasAttribute("class")) list << child;
        } else if (tag == "img") {
            list << child;
        } else {
            child.getChildren(list);
        }
        child = child.nextSibling();
    }
}

QString Fb2TextElement::location()
{
    static const QString javascript = FB2::read(":/js/get_location.js").prepend("var element=this;");
    return evaluateJavaScript(javascript).toString();
}

void Fb2TextElement::select()
{
    static const QString javascript = FB2::read(":/js/set_cursor.js");
    evaluateJavaScript(javascript);
}

bool Fb2TextElement::isBody() const
{
    return tagName() == "DIV" && attribute("class").toLower() == "body";
}

bool Fb2TextElement::isSection() const
{
    return tagName() == "DIV" && attribute("class").toLower() == "section";
}

bool Fb2TextElement::hasTitle() const
{
    return Fb2TextElement(firstChild()).isTitle();
}

bool Fb2TextElement::isTitle() const
{
    return tagName() == "DIV" && attribute("class").toLower() == "title";
}

//---------------------------------------------------------------------------
//  Fb2InsertCmd
//---------------------------------------------------------------------------

Fb2InsertCmd::Fb2InsertCmd(const Fb2TextElement &element)
    : QUndoCommand()
    , m_element(element)
    , m_parent(element.previousSibling())
    , m_inner(false)
{
    if (m_parent.isNull()) {
        m_parent = m_element.parent();
        m_inner = true;
    }
}

void Fb2InsertCmd::redo()
{
    if (m_inner) {
        m_parent.prependInside(m_element);
    } else {
        m_parent.appendOutside(m_element);
    }
    m_element.select();
}

void Fb2InsertCmd::undo()
{
    m_element.takeFromDocument();
}

//---------------------------------------------------------------------------
//  Fb2DeleteCmd
//---------------------------------------------------------------------------

Fb2DeleteCmd::Fb2DeleteCmd(const Fb2TextElement &element)
    : QUndoCommand()
    , m_element(element)
    , m_parent(element.previousSibling())
    , m_inner(false)
{
    if (m_parent.isNull()) {
        m_parent = element.parent();
        m_inner = true;
    }
}

void Fb2DeleteCmd::redo()
{
    m_element.takeFromDocument();
}

void Fb2DeleteCmd::undo()
{
    if (m_inner) {
        m_parent.prependInside(m_element);
    } else {
        m_parent.appendOutside(m_element);
    }
    m_element.select();
}

//---------------------------------------------------------------------------
//  Fb2MoveUpCmd
//---------------------------------------------------------------------------

Fb2MoveUpCmd::Fb2MoveUpCmd(const Fb2TextElement &element)
    : QUndoCommand()
    , m_element(element)
{
}

void Fb2MoveUpCmd::redo()
{
    Fb2TextElement subling = m_element.previousSibling();
    subling.prependOutside(m_element.takeFromDocument());
}

void Fb2MoveUpCmd::undo()
{
    Fb2TextElement subling = m_element.nextSibling();
    subling.appendOutside(m_element.takeFromDocument());
}

