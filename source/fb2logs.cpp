#include "fb2logs.hpp"

#include "fb2utils.h"

//---------------------------------------------------------------------------
//  FbLogModel::FbLogItem
//---------------------------------------------------------------------------

QVariant FbLogModel::FbLogItem::icon() const
{
    switch (m_type) {
        case QtDebugMsg: return FbIcon("dialog-information");
        case QtWarningMsg: return FbIcon("dialog-warning");
        case QtCriticalMsg: return FbIcon("dialog-error");
        case QtFatalMsg: return FbIcon("dialog-error");
    }
    return QVariant();
}

//---------------------------------------------------------------------------
//  FbLogModel
//---------------------------------------------------------------------------

FbLogModel::FbLogModel(QObject *parent)
    : QAbstractListModel(parent)
{
    for (FbLogItem *item: m_list) delete item;
}

QVariant FbLogModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0) return QVariant();
    if (row >= m_list.count()) return QVariant();
    switch (role) {
        case Qt::DisplayRole: return m_list.at(row)->msg();
        case Qt::DecorationRole: return m_list.at(row)->icon();
    }
    return QVariant();
}

int FbLogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_list.count();
}

void FbLogModel::add(QtMsgType type, int row, int col, const QString &msg)
{
    int count = m_list.count();
    QModelIndex parent = QModelIndex();
    beginInsertRows(parent, count, count);
    m_list.append(new FbLogItem(type, row, col, msg));
    endInsertRows();
    emit changeCurrent(createIndex(count, 0));
}

void FbLogModel::add(QtMsgType type, const QString &msg)
{
    add(type, 0, 0, msg);
}

//---------------------------------------------------------------------------
//  FbLogList
//---------------------------------------------------------------------------

FbLogList::FbLogList(QWidget *parent)
    : QListView(parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewMode(ListMode);
}

//---------------------------------------------------------------------------
//  FbLogDock
//---------------------------------------------------------------------------

FbLogDock::FbLogDock(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(title, parent, flags)
    , m_model(new FbLogModel(this))
    , m_list(new FbLogList(this))
{
    m_list->setModel(m_model);
    connect(m_model, SIGNAL(changeCurrent(QModelIndex)), m_list, SLOT(setCurrentIndex(QModelIndex)));
    setFeatures(QDockWidget::AllDockWidgetFeatures);
    setAttribute(Qt::WA_DeleteOnClose);
    setWidget(m_list);
}

void FbLogDock::append(QtMsgType type, const QString &message)
{
    m_model->add(type, message);
}

