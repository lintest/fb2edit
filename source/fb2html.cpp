#include "fb2html.h"
#include "fb2utils.h"

#include "fb2text.hpp"

//---------------------------------------------------------------------------
//  Fb2TextElement
//---------------------------------------------------------------------------

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

bool Fb2TextElement::isSection() const
{
    return tagName() == "DIV" && attribute("class").toLower() == "section";
}

bool Fb2TextElement::isTitle() const
{
    return tagName() == "DIV" && attribute("class").toLower() == "title";
}

//---------------------------------------------------------------------------
//  Fb2AddBodyCmd
//---------------------------------------------------------------------------

void Fb2AddBodyCmd::undo()
{
    m_page.body().lastChild().removeFromDocument();
    Fb2TextElement(m_page.body().lastChild()).select();
    m_page.update();
}

void Fb2AddBodyCmd::redo()
{
    m_page.body().appendInside("<div class=body><div class=section><p>text</p></div></div>");
    Fb2TextElement(m_page.body().lastChild()).select();
    m_page.update();
}

//---------------------------------------------------------------------------
//  Fb2SubtitleCmd
//---------------------------------------------------------------------------

void Fb2SubtitleCmd::redo()
{
    QString html = "<div class=subtitle><p><br/></p></div>";
    Fb2TextElement element = m_page.element(m_location);
    if (m_element.isNull()) {
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

