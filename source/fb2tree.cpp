#include "fb2tree.h"

#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFrame>

Fb2TreeModel::Fb2TreeModel(QTextEdit &text, QObject *parent)
    : QAbstractItemModel(parent)
    , m_text(text)
{
}

Fb2TreeModel::~Fb2TreeModel()
{
}

int Fb2TreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex Fb2TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    void *data = parent.isValid() ? (void*)frame(parent) : NULL;
    return createIndex(row, column, data);
}

QModelIndex Fb2TreeModel::parent(const QModelIndex &child) const
{
    if (QTextFrame * frame  = static_cast<QTextFrame*>(child.internalPointer())) {
        if (QTextFrame * parent = frame->parentFrame()) {
            int row = 0;
            foreach (QTextFrame * f, parent->childFrames()) {
                if (f == frame) return createIndex(row, 0, (void*)parent); else row++;
            }
        }
    }
    return QModelIndex();
}

int Fb2TreeModel::rowCount(const QModelIndex &parent) const
{
    QTextFrame * f = frame(parent);
    return f ? f->childFrames().count() : 0;
}

QTextFrame * Fb2TreeModel::frame(const QModelIndex &index) const
{
    if (!index.isValid()) {
        QTextDocument * doc = m_text.document();
        return doc ? doc->rootFrame() : NULL;
    }
    if (QTextFrame * parent = frame(index.parent())) {
        int i = index.row();
        foreach (QTextFrame * frame, parent->childFrames()) {
            if (i == 0) return frame; else i--;
        }
    }
    return NULL;
}

QVariant Fb2TreeModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();
    if (QTextFrame * f = frame(index)) {
        QTextCursor cursor(f);
        cursor.setPosition(f->firstPosition(), QTextCursor::MoveAnchor);
        cursor.setPosition(f->lastPosition(), QTextCursor::KeepAnchor);
        QTextDocumentFragment fragment(cursor);
        QString text = fragment.toPlainText();
        return text.simplified();
    }
    return tr("<section>");
}
