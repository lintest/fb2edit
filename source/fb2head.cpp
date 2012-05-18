#include "fb2head.h"

#include <QtDebug>
#include <QWebFrame>
#include <QWebPage>
#include <QWebView>
#include <QTreeView>

FB2_BEGIN_KEYHASH(Fb2HeadItem)
    FB2_KEY( Image  , "img"       );
    FB2_KEY( Seqn   , "sequence"  );
FB2_END_KEYHASH

Fb2HeadItem::Fb2HeadItem(QWebElement &element, Fb2HeadItem *parent)
    : QObject(parent)
    , m_element(element)
    , m_parent(parent)
{
    m_name = element.tagName().toLower();
    QString style = element.attribute("class").toLower();
    if (m_name == "div") {
        if (!style.isEmpty()) m_name = style;
    } else if (m_name == "img") {
        m_text = element.attribute("alt");
    }
    m_id = element.attribute("id");
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
        case 0: return QString("<%1>").arg(m_name);
        case 2: if (m_list.count() == 0) return value();
    }
    return QString();
}

QString Fb2HeadItem::value() const
{
    switch (toKeyword(m_name)) {
        case Image : {
            return m_element.attribute("alt");
        } break;
        case Seqn : {
            QString text = m_element.attribute("fb2:name");
            QString numb = m_element.attribute("fb2:number");
            if (numb.isEmpty() || numb == "0") return text;
            return text + ", " + tr("#") + numb;
        } break;
    }
    return m_element.toPlainText().simplified();
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
        if (node) view->expand(child);
    }
}

Fb2HeadItem * Fb2HeadModel::item(const QModelIndex &index) const
{
    if (index.isValid()) {
        return static_cast<Fb2HeadItem*>(index.internalPointer());
    } else {
        return m_root;
    }
}

int Fb2HeadModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

QModelIndex Fb2HeadModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!m_root || row < 0 || column < 0) return QModelIndex();
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
            }
        }
    }
    return QModelIndex();
}

int Fb2HeadModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) return 0;
    Fb2HeadItem *owner = item(parent);
    return owner ? owner->count() : 0;
}

QVariant Fb2HeadModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();
    Fb2HeadItem * i = item(index);
    return i ? i->text(index.column()) : QVariant();
}

QVariant Fb2HeadModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Tag");
            case 1: return tr("Description");
            case 2: return tr("Value");
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
