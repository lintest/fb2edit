#ifndef FB2HTML_H
#define FB2HTML_H

#include <QWebElement>

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

#endif // FB2HTML_H
