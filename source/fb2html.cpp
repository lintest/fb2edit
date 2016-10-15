#include "fb2html.h"
#include "fb2utils.h"
#include "fb2text.hpp"

//---------------------------------------------------------------------------
//  FbTextElement::Scheme
//---------------------------------------------------------------------------

FbTextElement::Scheme::Scheme()
{
    m_types["BODY"]
        << Type("FB:DESCRIPTION", 1, 1)
        << Type("FB:BODY", 1, 1)
    ;

    m_types["FB:DESCRIPTION"]
        << Type("FB:TITLE-INFO", 1, 1)
        << Type("FB:SRC-TITLE-INFO", 0, 1)
        << Type("FB:DOCUMENT-INFO", 1, 1)
        << Type("FB:PUBLISH-INFO", 0, 1)
        << Type("FB:CUSTOM-INFO")
    ;

    m_types["FB:DOCUMENT-INFO"]
        << Type("FB:AUTHOR", 1, 1)
        << Type("FB:PROGRAM-USED", 0, 1)
        << Type("FB:DATE", 0, 1)
    ;

    m_types["FB:BODY"]
        << Type("IMG")
        << Type("FB:TITLE", 0, 1)
        << Type("FB:EPIGRAPH")
        << Type("FB:SECTION", 1, 0)
    ;

    m_types["FB:SECTION"]
        << Type("FB:TITLE", 1, 0)
        << Type("FB:EPIGRAPH")
        << Type("IMG")
        << Type("FB:ANNOTATION")
        << Type("FB:SECTION")
    ;

    m_types["FB:POEM"]
        << Type("FB:TITLE", 1, 0)
        << Type("FB:EPIGRAPH", 0, 0)
        << Type("FB:STANZA", 1, 0)
    ;

    m_types["FB:STANZA"]
        << Type("FB:TITLE", 1, 0)
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
    while (m_pos != list.end()) {
        if (m_pos->name() == name) break;
        ++m_pos;
    }
}

FbTextElement::Sublist::operator bool() const
{
    return m_pos != m_list.end();
}

bool FbTextElement::Sublist::operator!() const
{
    return m_pos == m_list.end();
}

bool FbTextElement::Sublist::operator <(const FbTextElement &element) const
{
    if (element.isNull()) return true;
    const QString name = element.tagName();
    for (TypeList::const_iterator it = m_list.begin(); it != m_list.end(); ++it) {
        if (it->name() == name) return m_pos < it;
    }
    return false;
}

//---------------------------------------------------------------------------
//  FbTextElement
//---------------------------------------------------------------------------

FbTextElement FbTextElement::operator[](const QString &name)
{
    QString tagName = name.toUpper();
    FbTextElement child = firstChild();
    while (!child.isNull()) {
        if (child.tagName() == tagName) return child;
        child = child.nextSibling();
    }
    return insertInside(tagName, QString("<%1></%1>").arg(name));
}

QString FbTextElement::nodeName() const
{
    QString n = tagName().toLower();
    return n.left(3) == "fb:" ? n.mid(3) : n;
}

void FbTextElement::getChildren(FbElementList &list)
{
    FbTextElement child = firstChild();
    while (!child.isNull()) {
        QString tag = child.tagName();
        if (tag == "FB:DESCRIPTION") {
            // skip description
        } else if (tag.left(3) == "FB:") {
            list << child;
        } else if (tag == "IMG") {
            list << child;
        } else {
            child.getChildren(list);
        }
        child = child.nextSibling();
    }
}

int FbTextElement::childIndex() const
{
    FbElementList list;
    parent().getChildren(list);

    int result = 0;
    FbElementList::const_iterator it;
    for (it = list.constBegin(); it != list.constEnd(); ++it) {
        if (*it == *this) return result;
        ++result;
    }
    return -1;
}

bool FbTextElement::hasScheme() const
{
    return subtypes();
}

const FbTextElement::TypeList * FbTextElement::subtypes() const
{
    static Scheme scheme;
    return scheme[tagName()];
}

bool FbTextElement::hasSubtype(const QString &style) const
{
    if (const TypeList * list = subtypes()) {
        for (TypeList::const_iterator item = list->begin(); item != list->end(); ++item) {
            if (item->name() == style) return true;
        }
    }
    return false;
}

FbTextElement::TypeList::const_iterator FbTextElement::subtype(const TypeList &list, const QString &style)
{
    for (TypeList::const_iterator item = list.begin(); item != list.end(); ++item) {
        if (item->name() == style) return item;
    }
    return list.end();
}

FbTextElement FbTextElement::insertInside(const QString &style, const QString &html)
{
    const TypeList * types = subtypes();
    if (!types) return FbTextElement();

    Sublist sublist(*types, style);
    if (sublist) {
        FbTextElement child = firstChild();
        if (sublist < child) {
            prependInside(html);
            return firstChild();
        }
        while (!child.isNull()) {
            FbTextElement subling = child.nextSibling();
            if (sublist < subling) {
                child.appendOutside(html);
                return child.nextSibling();
            }
            child = subling;
        }
    }
    appendInside(html);
    return lastChild();
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
        if (child.tagName() == style) return true;
        child = child.nextSibling();
    }
    return false;
}

bool FbTextElement::isBody() const
{
    return tagName() == "FB:BODY";
}

bool FbTextElement::isSection() const
{
    return tagName() == "FB:SECTION";
}

bool FbTextElement::isTitle() const
{
    return tagName() == "FB:TITLE";
}

bool FbTextElement::isStanza() const
{
    return tagName() == "FB:STANZA";
}

bool FbTextElement::hasTitle() const
{
    return hasChild("FB:TITLE");
}

int FbTextElement::index() const
{
    int result = -1;
    FbTextElement prior = *this;
    while (!prior.isNull()) {
        prior = prior.previousSibling();
        ++result;
    }
    return result;
}

FbTextElement FbTextElement::child(int index) const
{
    FbTextElement result = firstChild();
    while (index > 0) {
        result = result.nextSibling();
        --index;
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
