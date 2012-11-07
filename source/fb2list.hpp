#ifndef FB2LIST_H
#define FB2LIST_H

#include <QTreeView>

class FbListView : public QTreeView
{
    Q_OBJECT

public:
    explicit FbListView(QWidget *parent = 0);

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

signals:
    void showCurrent(const QString &name);
};

#endif // FB2LIST_H
