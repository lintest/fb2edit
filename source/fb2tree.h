#ifndef FB2TREE_H
#define FB2TREE_H

#include <QAbstractItemModel>
#include <QTextEdit>

class Fb2TreeModel: public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit Fb2TreeModel(QTextEdit &text, QObject *parent = 0);
    virtual ~Fb2TreeModel();

public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

protected:
    QTextFrame * frame(const QModelIndex &index) const;

private:
    QTextEdit & m_text;
};

#endif // FB2TREE_H
