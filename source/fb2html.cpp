#include "fb2html.h"
#include "fb2utils.h"

#include "fb2text.hpp"

//---------------------------------------------------------------------------
//  FbTextElement
//---------------------------------------------------------------------------

void FbTextElement::getChildren(FbElementList &list)
{
    FbTextElement child = firstChild();
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

QString FbTextElement::location()
{
    static const QString javascript = FB2::read(":/js/get_location.js").prepend("var element=this;");
    return evaluateJavaScript(javascript).toString();
}

void FbTextElement::select()
{
    static const QString javascript = FB2::read(":/js/set_cursor.js");
    evaluateJavaScript(javascript);
}

bool FbTextElement::isBody() const
{
    return tagName() == "DIV" && attribute("class").toLower() == "body";
}

bool FbTextElement::isSection() const
{
    return tagName() == "DIV" && attribute("class").toLower() == "section";
}

bool FbTextElement::hasTitle() const
{
    return FbTextElement(firstChild()).isTitle();
}

bool FbTextElement::isTitle() const
{
    return tagName() == "DIV" && attribute("class").toLower() == "title";
}

//---------------------------------------------------------------------------
//  FbInsertCmd
//---------------------------------------------------------------------------

FbInsertCmd::FbInsertCmd(const FbTextElement &element)
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

void FbInsertCmd::redo()
{
    if (m_inner) {
        m_parent.prependInside(m_element);
    } else {
        m_parent.appendOutside(m_element);
    }
    m_element.select();
}

void FbInsertCmd::undo()
{
    m_element.takeFromDocument();
}

//---------------------------------------------------------------------------
//  FbDeleteCmd
//---------------------------------------------------------------------------

FbDeleteCmd::FbDeleteCmd(const FbTextElement &element)
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

void FbDeleteCmd::redo()
{
    m_element.takeFromDocument();
}

void FbDeleteCmd::undo()
{
    if (m_inner) {
        m_parent.prependInside(m_element);
    } else {
        m_parent.appendOutside(m_element);
    }
    m_element.select();
}

//---------------------------------------------------------------------------
//  FbMoveUpCmd
//---------------------------------------------------------------------------

FbMoveUpCmd::FbMoveUpCmd(const FbTextElement &element)
    : QUndoCommand()
    , m_element(element)
{
}

void FbMoveUpCmd::redo()
{
    FbTextElement subling = m_element.previousSibling();
    subling.prependOutside(m_element.takeFromDocument());
}

void FbMoveUpCmd::undo()
{
    FbTextElement subling = m_element.nextSibling();
    subling.appendOutside(m_element.takeFromDocument());
}

