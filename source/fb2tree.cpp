#include "fb2tree.h"

#include <QtDebug>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFrame>

Fb2TreeItem::Fb2TreeItem(QTextFrame *frame, Fb2TreeItem *parent)
    : QObject(parent)
    , m_frame(frame)
    , m_parent(parent)
{
    QTextCursor cursor(m_frame);
    cursor.setPosition(m_frame->firstPosition(), QTextCursor::MoveAnchor);
    cursor.setPosition(m_frame->lastPosition(), QTextCursor::KeepAnchor);
    QTextDocumentFragment fragment(cursor);
    m_text = fragment.toPlainText().simplified().left(255);
    foreach (QTextFrame * frame, frame->childFrames()) {
        m_list << new Fb2TreeItem(frame, this);
    }
}

Fb2TreeItem::~Fb2TreeItem()
{
    foreach (Fb2TreeItem * item, m_list) {
        delete item;
    }
}

Fb2TreeItem * Fb2TreeItem::item(int index) const
{
    if (index < 0 || index >= m_list.size()) return NULL;
    return m_list[index];
}

QString Fb2TreeItem::text() const
{
    return tr("<section>") + m_text;
}

//---------------------------------------------------------------------------
//  Fb2TreeModel
//---------------------------------------------------------------------------

Fb2TreeModel::Fb2TreeModel(QTextEdit &text, QObject *parent)
    : QAbstractItemModel(parent)
    , m_text(text)
    , m_root(NULL)
{
    if (QTextDocument * doc = text.document()) {
        m_root = new Fb2TreeItem(doc->rootFrame());
    }
}

Fb2TreeModel::~Fb2TreeModel()
{
    if (m_root) delete m_root;
}

Fb2TreeItem * Fb2TreeModel::item(const QModelIndex &index) const
{
    if (!m_root) return NULL;
    if (!index.isValid()) return m_root;
    Fb2TreeItem * parent = item(index.parent());
    return parent ? parent->item(index.row()) : NULL;
}

int Fb2TreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex Fb2TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!m_root) return QModelIndex();
    Fb2TreeItem * i  = item(parent);
    return i ? createIndex(row, column, (void*)i) : QModelIndex();
}

QModelIndex Fb2TreeModel::parent(const QModelIndex &child) const
{
    if (Fb2TreeItem * parent = static_cast<Fb2TreeItem*>(child.internalPointer())) {
        if (Fb2TreeItem * owner = parent->parent()) {
            return createIndex(owner->index(parent), 0, (void*)owner);
        }
    }
    return QModelIndex();
}

int Fb2TreeModel::rowCount(const QModelIndex &parent) const
{
    Fb2TreeItem * i  = item(parent);
    return i ? i->count() : 0;
}

QVariant Fb2TreeModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();
    Fb2TreeItem * i  = item(index);
    return i ? i->text() : QVariant();
}

void Fb2TreeModel::select(const QModelIndex &index)
{
    Fb2TreeItem * i = item(index);
    if (!i) return;

    QTextFrame * f = i->frame();
    if (!f) return;

    QTextCursor cursor = m_text.textCursor();
    cursor.setPosition(f->firstPosition());
    m_text.moveCursor(QTextCursor::End);
    m_text.setTextCursor(cursor);
}
