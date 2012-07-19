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
//  Fb2UndoCommand
//---------------------------------------------------------------------------

QString Fb2UndoCommand::div(const QString &style, const QString &text)
{
    return QString("<div class=%1>%2</div>").arg(style).arg(text);
}

QString Fb2UndoCommand::p(const QString &text)
{
    return QString("<p>%1</p>").arg(text);
}

//---------------------------------------------------------------------------
//  Fb2AddBodyCmd
//---------------------------------------------------------------------------

void Fb2AddBodyCmd::undo()
{
    m_body.takeFromDocument();
    m_page.update();
}

void Fb2AddBodyCmd::redo()
{
    Fb2TextElement parent = m_page.body();
    if (m_body.isNull()) {
        QString html = div("body", div("title", p()) + div("section", div("title", p()) + p()));
        parent.appendInside(html);
        m_body = parent.lastChild();
    } else {
        parent.appendInside(m_body);
    }
    m_body.select();
    m_page.update();
}

//---------------------------------------------------------------------------
//  Fb2SectionCmd
//---------------------------------------------------------------------------

void Fb2SectionCmd::redo()
{
    if (m_child.isNull()) {
        QString html = div("section", div("title", p()) + p());
        m_parent.appendInside(html);
        m_child = m_parent.lastChild();
    } else {
        m_parent.appendInside(m_child);
    }
    m_child.select();
    m_page.update();
}

void Fb2SectionCmd::undo()
{
    m_child.takeFromDocument();
    Fb2TextElement last = m_parent.lastChild();
    if (last.isNull()) {
        m_parent.select();
    } else {
        last.select();
    }
    m_page.update();
}

//---------------------------------------------------------------------------
//  Fb2TitleCmd
//---------------------------------------------------------------------------

void Fb2TitleCmd::redo()
{
    if (m_title.isNull()) {
        QString html = div("title", p());
        m_section.prependInside(html);
        m_title = m_section.firstChild();
    } else {
        m_section.prependInside(m_title);
    }
    m_section.select();
    m_page.update();
}

void Fb2TitleCmd::undo()
{
    m_title.takeFromDocument();
    m_section.select();
    m_page.update();
}

//---------------------------------------------------------------------------
//  Fb2SubtitleCmd
//---------------------------------------------------------------------------

void Fb2SubtitleCmd::redo()
{
    Fb2TextElement element = m_page.element(m_location);
    if (m_element.isNull()) {
        QString html = div("subtitle", p());
        element.appendOutside(html);
    } else {
        element.appendOutside(m_element);
    }
    element = element.nextSibling();
    m_position = element.location();
    element.select();
    m_page.update();
}

void Fb2SubtitleCmd::undo()
{
    Fb2TextElement element = m_page.element(m_position);
    Fb2TextElement parent = element.parent();
    m_element = element.takeFromDocument();
    parent.select();
    m_page.update();
}

//---------------------------------------------------------------------------
//  Fb2DeleteCmd
//---------------------------------------------------------------------------

Fb2DeleteCmd::Fb2DeleteCmd(Fb2TextPage &page, const Fb2TextElement &element, Fb2UndoCommand *parent)
    : Fb2UndoCommand(parent)
    , m_element(element)
    , m_page(page)
    , m_inner(false)
{
    m_parent = element.previousSibling();
    if (m_parent.isNull()) {
        m_parent = element.parent();
        m_inner = true;
    }
}

void Fb2DeleteCmd::redo()
{
    m_element.takeFromDocument();
    m_page.update();
}

void Fb2DeleteCmd::undo()
{
    if (m_inner) {
        m_parent.prependInside(m_element);
    } else {
        m_parent.appendOutside(m_element);
    }
    m_page.update();
}

