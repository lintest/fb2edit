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

