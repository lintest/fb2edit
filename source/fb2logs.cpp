#include "fb2logs.hpp"

//---------------------------------------------------------------------------
//  FbLogItem
//---------------------------------------------------------------------------

FbLogItem::FbLogItem(Level level, int row, int col, const QString &msg)
    : m_level(level)
    , m_msg(msg)
    , m_row(row)
    , m_col(col)
{
}

FbLogItem::FbLogItem(Level level, const QString &msg)
    : m_level(level)
    , m_msg(msg)
    , m_row(0)
    , m_col(0)
{
}

//---------------------------------------------------------------------------
//  FbLogList
//---------------------------------------------------------------------------

FbLogModel::FbLogModel(QObject *parent)
    : QAbstractListModel(parent)
{
    foreach (FbLogItem *item, m_list) delete item;
}

QVariant FbLogModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0) return QVariant();
    if (row >= m_list.count()) return QVariant();
    return QVariant();
}

int FbLogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_list.count();
}

//---------------------------------------------------------------------------
//  FbLogList
//---------------------------------------------------------------------------

FbLogList::FbLogList(QWidget *parent)
    : QListView(parent)
{
    setModel(new FbLogModel(this));
    setViewMode(ListMode);
}

//---------------------------------------------------------------------------
//  FbLogDock
//---------------------------------------------------------------------------

FbLogDock::FbLogDock(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(title, parent, flags)
    , m_list(new FbLogList(this))
{
    setFeatures(QDockWidget::AllDockWidgetFeatures);
    setAttribute(Qt::WA_DeleteOnClose);
    setWidget(m_list);
}

void FbLogDock::append(const QString &message)
{
}

