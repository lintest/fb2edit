#include "fb2html.h"
#include "fb2utils.h"

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
