#ifndef FB2TREE_H
#define FB2TREE_H

#include <QAbstractItemModel>
#include <QWebElement>
#include <QWebView>

QT_BEGIN_NAMESPACE
class QTreeView;
QT_END_NAMESPACE

class Fb2TreeItem: public QObject
{
    Q_OBJECT

public:
    explicit Fb2TreeItem(QWebElement &element, Fb2TreeItem *parent = 0);

    virtual ~Fb2TreeItem();

    Fb2TreeItem * item(const QModelIndex &index) const;

    Fb2TreeItem * item(int row) const;

    int index(Fb2TreeItem * child) const {
        return m_list.indexOf(child);
    }

    int count() const {
        return m_list.size();
    }

    Fb2TreeItem * parent() const {
        return m_parent;
    }

    QString text() const;

    const QString & name() const {
        return m_name;
    }

    QPoint pos() const {
        return m_element.geometry().topLeft();
    }

    QString selector() const;

private:
    QString static title(const QWebElement &element);
    void addChildren(QWebElement &parent);

private:
    QList<Fb2TreeItem*> m_list;
    QWebElement m_element;
    QString m_name;
    QString m_text;
    Fb2TreeItem * m_parent;
};

class Fb2TreeModel: public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit Fb2TreeModel(QWebView &view, QObject *parent = 0);
    virtual ~Fb2TreeModel();
    void select(const QModelIndex &index);
    void expand(QTreeView *view);

public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

protected:
    Fb2TreeItem * item(const QModelIndex &index) const;

private:
    QWebView & m_view;
    Fb2TreeItem * m_root;
};

#endif // FB2TREE_H
