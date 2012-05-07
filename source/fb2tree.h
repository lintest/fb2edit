#ifndef FB2TREE_H
#define FB2TREE_H

#include <QAbstractItemModel>
#include <QTextEdit>
#include <QWebElement>
#include <QWebView>

class Fb2TreeItem: public QObject
{
    Q_OBJECT

public:
    explicit Fb2TreeItem(QTextFrame *frame, Fb2TreeItem *parent = 0);
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

    QTextFrame * frame() const {
        return m_frame;
    }

    QString text() const;

private:
    QList<Fb2TreeItem*> m_list;
    QString m_name;
    QString m_text;
    QTextFrame * m_frame;
    Fb2TreeItem * m_parent;
    QWebElement m_element;
};

class Fb2TreeModel: public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit Fb2TreeModel(QWebView &view, QObject *parent = 0);
    virtual ~Fb2TreeModel();
    void select(const QModelIndex &index);

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
