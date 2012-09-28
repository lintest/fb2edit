#include "fb2html.h"
#include "fb2utils.h"
#include "fb2text.hpp"

//---------------------------------------------------------------------------
//  FbTextElement::Scheme
//---------------------------------------------------------------------------

FbTextElement::Scheme::Scheme()
{
    m_types["body"]
        << Type("image")
        << Type("title")
        << Type("epigraph", 0, 0)
        << Type()
    ;

    m_types["section"]
        << Type("title")
        << Type("epigraph", 0, 0)
        << Type("image")
        << Type("annotation")
        << Type()
    ;

    m_types["poem"]
        << Type("title")
        << Type("epigraph", 0, 0)
        << Type("stanza", 1, 0)
        << Type()
        << Type("text-author", 0, 0)
        << Type("date")
    ;

    m_types["stanza"]
        << Type("title")
        << Type("subtitle")
        << Type()
    ;

    m_types["epigraph"]
        << Type()
        << Type("text-author", 0, 0)
    ;

    m_types["cite"]
        << Type()
        << Type("text-author", 0, 0)
    ;
}

const FbTextElement::TypeList * FbTextElement::Scheme::operator[](const QString &name) const
{
    TypeMap::const_iterator it = m_types.find(name);
    if (it != m_types.end()) return &it.value();
    return 0;
}

//---------------------------------------------------------------------------
//  FbTextElement::Sublist
//---------------------------------------------------------------------------

FbTextElement::Sublist::Sublist(const TypeList &list, const QString &name)
    : m_list(list)
    , m_pos(list.begin())
{
    TypeList::const_iterator empty = list.end();
    while (m_pos != list.end()) {
        if (m_pos->name() == name) break;
        if (m_pos->name().isEmpty()) empty = m_pos;
        m_pos++;
    }
    if (m_pos == list.end()) m_pos = empty;
}

FbTextElement::Sublist::operator bool() const
{
    return m_pos != m_list.end();
}

bool FbTextElement::Sublist::operator!() const
{
    return m_pos == m_list.end();
}

bool FbTextElement::Sublist::operator <(const QWebElement &element) const
{
    const QString name = element.attribute("class");
    for (TypeList::const_iterator it = m_list.begin(); it != m_list.end(); it++) {
        if (it->name() == name) return m_pos < it  || element.isNull();
    }
    return false;
}

bool FbTextElement::Sublist::operator >=(const QWebElement &element) const
{
    const QString name = element.attribute("class");
    for (TypeList::const_iterator it = m_list.begin(); it != m_list.end(); it++) {
        if (it->name() == name) return m_pos >= it || element.isNull();
    }
    return false;
}

bool FbTextElement::Sublist::operator !=(const QWebElement &element) const
{
    return element.isNull() || m_pos->name() != element.attribute("class");
}

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

bool FbTextElement::hasScheme() const
{
    return subtypes();
}

const FbTextElement::TypeList * FbTextElement::subtypes() const
{
    static Scheme scheme;
    return scheme[attribute("class").toLower()];
}

bool FbTextElement::hasSubtype(const QString &style) const
{
    if (const TypeList * list = subtypes()) {
        for (TypeList::const_iterator item = list->begin(); item != list->end(); item++) {
            if (item->name() == style) return true;
        }
    }
    return false;
}

FbTextElement::TypeList::const_iterator FbTextElement::subtype(const TypeList &list, const QString &style)
{
    for (TypeList::const_iterator item = list.begin(); item != list.end(); item++) {
        if (item->name() == style) return item;
    }
    return list.end();
}

FbTextElement FbTextElement::insertInside(const QString &style, const QString &html)
{
    const TypeList * types = subtypes();
    if (!types) return FbTextElement();

    Sublist sublist(*types, style);
    if (!sublist) return FbTextElement();

    FbTextElement child = firstChild();
    if (sublist < child) {
        prependInside(html);
        return firstChild();
    }

    while (!child.isNull()) {
        FbTextElement subling = child.nextSibling();
        if (sublist >= child && sublist != subling) {
            child.appendOutside(html);
            return child.nextSibling();
        }
        child = subling;
    }

    return FbTextElement();
}

QString FbTextElement::location()
{
    return evaluateJavaScript("location(this)").toString();
}

void FbTextElement::select()
{
    QString javascript = jScript("set_cursor.js");
    evaluateJavaScript(javascript);
}

bool FbTextElement::hasChild(const QString &style) const
{
    FbTextElement child = firstChild();
    while (!child.isNull()) {
        if (child.tagName() == "DIV" && child.attribute("class").toLower() == style) return true;
        child = child.nextSibling();
    }
    return false;
}

bool FbTextElement::isDiv(const QString &style) const
{
    return tagName() == "DIV" && attribute("class").toLower() == style;
}

bool FbTextElement::isBody() const
{
    return isDiv("body");
}

bool FbTextElement::isSection() const
{
    return isDiv("section");
}

bool FbTextElement::isTitle() const
{
    return isDiv("title");
}

bool FbTextElement::isStanza() const
{
    return isDiv("stanza");
}

bool FbTextElement::hasTitle() const
{
    return FbTextElement(firstChild()).isTitle();
}

int FbTextElement::index() const
{
    int result = -1;
    FbTextElement prior = *this;
    while (!prior.isNull()) {
        prior = prior.previousSibling();
        result++;
    }
    return result;
}

FbTextElement FbTextElement::child(int index) const
{
    FbTextElement result = firstChild();
    while (index > 0) {
        result = result.nextSibling();
        index--;
    }
    return index ? FbTextElement() : result;
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
//  FbReplaceCmd
//---------------------------------------------------------------------------

FbReplaceCmd::FbReplaceCmd(const FbTextElement &original, const FbTextElement &duplicate)
    : QUndoCommand()
    , m_original(original)
    , m_duplicate(duplicate)
    , m_update(false)
{
}

void FbReplaceCmd::redo()
{
    if (m_update) {
        m_original.prependOutside(m_duplicate);
        m_original.takeFromDocument();
        m_duplicate.select();
    } else {
        m_update = true;
    }
}

void FbReplaceCmd::undo()
{
    m_duplicate.prependOutside(m_original);
    m_duplicate.takeFromDocument();
    m_original.select();
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


//---------------------------------------------------------------------------
//  FbMoveLeftCmd
//---------------------------------------------------------------------------

FbMoveLeftCmd::FbMoveLeftCmd(const FbTextElement &element)
    : QUndoCommand()
    , m_element(element)
    , m_subling(element.previousSibling())
    , m_parent(element.parent())
{
}

void FbMoveLeftCmd::redo()
{
    m_parent.appendOutside(m_element.takeFromDocument());
}

void FbMoveLeftCmd::undo()
{
    if (m_subling.isNull()) {
        m_parent.prependInside(m_element.takeFromDocument());
    } else {
        m_subling.appendOutside(m_element.takeFromDocument());
    }
}

//---------------------------------------------------------------------------
//  FbMoveRightCmd
//---------------------------------------------------------------------------

FbMoveRightCmd::FbMoveRightCmd(const FbTextElement &element)
    : QUndoCommand()
    , m_element(element)
    , m_subling(element.previousSibling())
{
}

void FbMoveRightCmd::redo()
{
    m_subling.appendInside(m_element.takeFromDocument());
}

void FbMoveRightCmd::undo()
{
    m_subling.appendOutside(m_element.takeFromDocument());
}

