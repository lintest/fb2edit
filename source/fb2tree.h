#ifndef FB2TREE_H
#define FB2TREE_H

#include <QWebElement>

class Fb2WebElement : public QWebElement
{
public:
    Fb2WebElement() {}
    Fb2WebElement(const QWebElement &x) : QWebElement(x) {}
    Fb2WebElement &operator=(const QWebElement &x) { QWebElement::operator=(x); return *this; }

public:
    void select();
};

#endif // FB2TREE_H
