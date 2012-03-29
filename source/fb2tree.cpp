#include "fb2tree.h"

Fb2TreeModel::Fb2TreeModel(QTextDocument &document, QObject *parent)
    : QAbstractItemModel(parent)
    , m_document(document)
{
}

Fb2TreeModel::~Fb2TreeModel()
{
}
