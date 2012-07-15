#include "fb2html.h"
#include "fb2utils.h"

//---------------------------------------------------------------------------
//  Fb2WebElement
//---------------------------------------------------------------------------

void Fb2WebElement::select()
{
    static const QString javascript = FB2::read(":/js/set_cursor.js");
    evaluateJavaScript(javascript);
}

bool Fb2WebElement::isSection() const
{
    return tagName() == "DIV" && attribute("class").toLower() == "section";
}

bool Fb2WebElement::isTitle() const
{
    return tagName() == "DIV" && attribute("class").toLower() == "title";
}
