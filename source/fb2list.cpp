#include "fb2list.hpp"

//---------------------------------------------------------------------------
//  FbListView
//---------------------------------------------------------------------------

FbListView::FbListView(QWidget *parent)
    : QTreeView(parent)
{
    setAllColumnsShowFocus(true);
    setRootIsDecorated(false);
}

void FbListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTreeView::currentChanged(current, previous);
    QModelIndex index = model()->index(current.row(), 0);
    emit showCurrent(model()->data(index).toString());
}

