#include "fb2head.hpp"

#include <QtDebug>
#include <QAction>
#include <QHeaderView>
#include <QToolBar>
#include <QWebFrame>
#include <QWebPage>
#include <QWebView>
#include <QItemDelegate>
#include <QTreeView>

#include "fb2view.hpp"
#include "fb2utils.h"

//---------------------------------------------------------------------------
//  Fb2Scheme::Fb2
//---------------------------------------------------------------------------

Fb2Scheme::Fb2::Fb2()
{
    QFile file(":/fb2/FictionBook2.1.xsd");
    if (file.open(QIODevice::ReadOnly)) setContent(&file);
}

//---------------------------------------------------------------------------
//  Fb2Scheme
//---------------------------------------------------------------------------

const QDomDocument & Fb2Scheme::fb2()
{
    static const Fb2 doc;
    return doc;
}

FB2_BEGIN_KEYHASH(Fb2Scheme)
    FB2_KEY( Element , "xs:element"     );
    FB2_KEY( Content , "xs:choice"      );
    FB2_KEY( Content , "xs:complexType" );
    FB2_KEY( Content , "xs:sequence"    );
FB2_END_KEYHASH

QString Fb2Scheme::info() const
{
    QDomElement element = *this;
    if (element.isNull()) return QString();

    element = element.firstChildElement("xs:annotation");
    if (element.isNull()) return QString();

    element = element.firstChildElement("xs:documentation");
    if (element.isNull()) return QString();

    return element.text();
}

QString Fb2Scheme::type() const
{
    QString result = attribute("type");
    if (!result.isEmpty()) return result;
    Fb2Scheme child = firstChildElement("xs:complexType");
    child = child.firstChildElement("xs:complexContent");
    child = child.firstChildElement("xs:extension");
    return child.attribute("base");
}

Fb2Scheme Fb2Scheme::element(const QString &name) const
{
    Fb2Scheme parent = *this;
    if (parent.isNull()) {
        parent = fb2().documentElement();
        parent = parent.element("FictionBook");
    }

    Fb2Scheme child = parent.firstChildElement();
    while (!child.isNull()) {
        switch (toKeyword(child.tagName())) {
            case Element: {
                    if (child.attribute("name") == name) return child;
                } break;
            case Content: {
                    Fb2Scheme result = child.element(name);
                    if (!result.isNull()) return result;
                } break;
            default: ;
        }
        child = child.nextSiblingElement();
    }

    QString type = this->type();
    if (type.isEmpty()) return QDomElement();

    child = fb2().firstChildElement().firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == "xs:complexType") {
            if (child.attribute("name") == type) return child.element(name);
        }
        child = child.nextSiblingElement();
    }

    return QDomElement();
}

//---------------------------------------------------------------------------
//  Fb2HeadItem
//---------------------------------------------------------------------------

Fb2HeadItem::HintHash::HintHash()
{
    insert( "title-info"    , tr( "Book"        ));
    insert( "document-info" , tr( "File"        ));
    insert( "publish-info"  , tr( "Publish"     ));
    insert( "custom-info"   , tr( "Add-ons"     ));
    insert( "genre"         , tr( "Genre"       ));
    insert( "author"        , tr( "Author"      ));
    insert( "book-title"    , tr( "Title"       ));
    insert( "annotation"    , tr( "Annotation"  ));
    insert( "coverpage"     , tr( "Cover"       ));
    insert( "date"          , tr( "Date"        ));
    insert( "lang"          , tr( "Language"    ));
    insert( "translator"    , tr( "Translator"  ));
    insert( "sequence"      , tr( "Sequence"    ));
    insert( "first-name"    , tr( "First name"  ));
    insert( "middle-name"   , tr( "Middle name" ));
    insert( "last-name"     , tr( "Last name"   ));
    insert( "history"       , tr( "History"     ));
}

FB2_BEGIN_KEYHASH(Fb2HeadItem)
    FB2_KEY( Auth   , "author"    );
    FB2_KEY( Cover  , "coverpage" );
    FB2_KEY( Image  , "img"       );
    FB2_KEY( Seqn   , "sequence"  );
FB2_END_KEYHASH

Fb2HeadItem::Fb2HeadItem(QWebElement &element, Fb2HeadItem *parent)
    : QObject(parent)
    , m_element(element)
    , m_parent(parent)
{
    m_name = element.tagName().toLower();
    m_id = element.attribute("id");
    if (m_name == "div") {
        QString style = element.attribute("class").toLower();
        if (!style.isEmpty()) m_name = style;
        if (style == "annotation") return;
        if (style == "history") return;
    } else if (m_name == "img") {
        m_text = element.attribute("alt");
    }
    addChildren(element);
}

Fb2HeadItem::~Fb2HeadItem()
{
    foreach (Fb2HeadItem * item, m_list) {
        delete item;
    }
}

void Fb2HeadItem::addChildren(QWebElement &parent)
{
    QWebElement child = parent.firstChild();
    while (!child.isNull()) {
        QString tag = child.tagName().toLower();
        if (tag == "div") {
            m_list << new Fb2HeadItem(child, this);
        } else if (tag == "img") {
            m_list << new Fb2HeadItem(child, this);
        } else {
            addChildren(child);
        }
        child = child.nextSibling();
    }
}

Fb2HeadItem * Fb2HeadItem::item(const QModelIndex &index) const
{
    int row = index.row();
    if (row < 0 || row >= m_list.size()) return NULL;
    return m_list[row];
}

Fb2HeadItem * Fb2HeadItem::item(int row) const
{
    if (row < 0 || row >= m_list.size()) return NULL;
    return m_list[row];
}

QString Fb2HeadItem::text(int col) const
{
    switch (col) {
        case 0: return QString("<%1> %2").arg(m_name).arg(hint());
        case 1: return value();
        case 2: return scheme().info();
        case 3: return scheme().type();
        case 4: return scheme().attribute("minOccurs");
        case 5: return scheme().attribute("maxOccurs");
    }
    return QString();
}

QString Fb2HeadItem::hint() const
{
    static HintHash hints;
    HintHash::const_iterator it = hints.find(m_name);
    if (it == hints.end()) return QString();
    return it.value();
}

QString Fb2HeadItem::value() const
{
    switch (toKeyword(m_name)) {
        case Auth : {
            QString result = sub("last-name");
            result += " " + sub("first-name");
            result += " " + sub("middle-name");
            return result.simplified();
        } break;
        case Cover : {
            QString text;
            foreach (Fb2HeadItem * item, m_list) {
                if (item->m_name == "img") {
                    if (!text.isEmpty()) text += ", ";
                    text += item->value();
                }
            }
            return text;
        } break;
        case Image : {
            return m_element.attribute("src");
        } break;
        case Seqn : {
            QString text = m_element.attribute("fb2.name");
            QString numb = m_element.attribute("fb2.number");
            if (numb.isEmpty() || numb == "0") return text;
            return text + ", " + tr("#") + numb;
        } break;
        default: ;
    }
    if (m_list.count()) return QString();
    return m_element.toPlainText().simplified();
}

QString Fb2HeadItem::sub(const QString &key) const
{
    foreach (Fb2HeadItem * item, m_list) {
        if (item->m_name == key) return item->value();
    }
    return QString();
}

Fb2Scheme Fb2HeadItem::scheme() const
{
    Fb2Scheme parent = m_parent ? m_parent->scheme() : Fb2Scheme();
    return parent.element(m_name);
}

//---------------------------------------------------------------------------
//  Fb2HeadModel
//---------------------------------------------------------------------------

Fb2HeadModel::Fb2HeadModel(QWebView &view, QObject *parent)
    : QAbstractItemModel(parent)
    , m_view(view)
    , m_root(NULL)
{
    QWebElement doc = view.page()->mainFrame()->documentElement();
    QWebElement head = doc.findFirst("div.description");
    if (head.isNull()) return;
    m_root = new Fb2HeadItem(head);
}

Fb2HeadModel::~Fb2HeadModel()
{
    if (m_root) delete m_root;
}

void Fb2HeadModel::expand(QTreeView *view)
{
    QModelIndex parent = QModelIndex();
    int count = rowCount(parent);
    for (int i = 0; i < count; i++) {
        QModelIndex child = index(i, 0, parent);
        Fb2HeadItem *node = item(child);
        if (!node) continue;
        view->expand(child);
        int count = rowCount(child);
        for (int j = 0; j < count; j++) {
            QModelIndex ch = index(j, 0, child);
            Fb2HeadItem *node = item(ch);
            if (node) view->expand(ch);
        }
    }
}

Fb2HeadItem * Fb2HeadModel::item(const QModelIndex &index) const
{
    if (index.isValid()) {
        return static_cast<Fb2HeadItem*>(index.internalPointer());
    } else {
        return 0;
    }
}

int Fb2HeadModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 6;
}

QModelIndex Fb2HeadModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!m_root || row < 0 || column < 0) return QModelIndex();

    if (!parent.isValid() && m_root) {
        return createIndex(row, column, (void*)m_root);
    }

    if (Fb2HeadItem *owner = item(parent)) {
        if (Fb2HeadItem *child = owner->item(row)) {
            return createIndex(row, column, (void*)child);
        }
    }

    return QModelIndex();
}

QModelIndex Fb2HeadModel::parent(const QModelIndex &child) const
{
    if (Fb2HeadItem * node = static_cast<Fb2HeadItem*>(child.internalPointer())) {
        if (Fb2HeadItem * parent = node->parent()) {
            if (Fb2HeadItem * owner = parent->parent()) {
                return createIndex(owner->index(parent), 0, (void*)parent);
            } else {
                return createIndex(0, 0, (void*)parent);
            }
        }
    }
    return QModelIndex();
}

int Fb2HeadModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) return 0;
    if (!parent.isValid()) return m_root ? 1 : 0;
    Fb2HeadItem *owner = item(parent);
    return owner ? owner->count() : 0;
}

QVariant Fb2HeadModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();
    Fb2HeadItem * i = item(index);
    return i ? i->text(index.column()) : QVariant();
}

QVariant Fb2HeadModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Key");
            case 1: return tr("Value");
        }
    }
    return QVariant();
}

void Fb2HeadModel::select(const QModelIndex &index)
{
    Fb2HeadItem *node = item(index);
    if (!node || node->id().isEmpty()) return;
    m_view.page()->mainFrame()->scrollToAnchor(node->id());
}

void Fb2HeadItem::setText(const QString &text)
{
    m_text = text;
}

bool Fb2HeadModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) return false;
    Fb2HeadItem * i = item(index);
    if (!i) return false;
    i->setText(value.toString());
    emit dataChanged(index, index);
    return true;
}

Qt::ItemFlags Fb2HeadModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return 0;
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == 1) flags |= Qt::ItemIsEditable;
    return flags;
}

//---------------------------------------------------------------------------
//  Fb2TreeView
//---------------------------------------------------------------------------

Fb2HeadView::Fb2HeadView(Fb2WebView &view, QWidget *parent)
    : QTreeView(parent)
    , m_view(view)
{
    QAction * act;

    actionInsert = act = new QAction(FB2::icon("list-add"), tr("&Append"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::New);

    actionModify = act = new QAction(FB2::icon("list-add"), tr("&Modify"), this);
    act->setPriority(QAction::LowPriority);

    actionDelete = act = new QAction(FB2::icon("list-remove"), tr("&Delete"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Delete);

    //setItemDelegate(new QItemDelegate(this));
    setRootIsDecorated(false);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(&m_view, SIGNAL(loadFinished(bool)), SLOT(updateTree()));
    connect(this, SIGNAL(activated(QModelIndex)), SLOT(activated(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)), SLOT(collapsed(QModelIndex)));

    header()->setDefaultSectionSize(200);
    connect(actionModify, SIGNAL(triggered()), SLOT(editCurrent()));
}

void Fb2HeadView::initToolbar(QToolBar &toolbar)
{
    toolbar.addSeparator();
    toolbar.addAction(actionInsert);
    toolbar.addAction(actionDelete);
}

void Fb2HeadView::updateTree()
{
    Fb2HeadModel * model = new Fb2HeadModel(m_view, this);
    setModel(model);
    model->expand(this);
}

void Fb2HeadView::editCurrent()
{
    if (!model()) return;
    QModelIndex current = currentIndex();
    if (!current.isValid()) return;
    QModelIndex parent = model()->parent(current);
    QModelIndex index = model()->index(current.row(), 1, parent);
    setCurrentIndex(index);
    edit(index);
}

void Fb2HeadView::activated(const QModelIndex &index)
{
    if (index.isValid() && index.column() == 1) edit(index);
    showStatus(index);
}

void Fb2HeadView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTreeView::currentChanged(current, previous);
    showStatus(current);
}

void Fb2HeadView::showStatus(const QModelIndex &current)
{
    if (!model()) return;
    if (!current.isValid()) return;
    QModelIndex parent = model()->parent(current);
    QModelIndex index = model()->index(current.row(), 2, parent);
    emit status(model()->data(index).toString());
}

void Fb2HeadView::collapsed(const QModelIndex &index)
{
    if (model() && !model()->parent(index).isValid()) {
        expand(index);
    }
}
