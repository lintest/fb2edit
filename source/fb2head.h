#ifndef FB2HEAD_H
#define FB2HEAD_H

#include <QAbstractItemModel>
#include <QWebElement>
#include <QWebView>

QT_BEGIN_NAMESPACE
class QTreeView;
QT_END_NAMESPACE

#include "fb2xml.h"

class Fb2HeadItem: public QObject
{
    Q_OBJECT

    FB2_BEGIN_KEYLIST
        Image,
        Seqn,
    FB2_END_KEYLIST

public:
    explicit Fb2HeadItem(QWebElement &element, Fb2HeadItem *parent = 0);

    virtual ~Fb2HeadItem();

    Fb2HeadItem * item(const QModelIndex &index) const;

    Fb2HeadItem * item(int row) const;

    int index(Fb2HeadItem * child) const {
        return m_list.indexOf(child);
    }

    int count() const {
        return m_list.size();
    }

    Fb2HeadItem * parent() const {
        return m_parent;
    }

    QString text(int col = 0) const;

    const QString & id() const {
        return m_id;
    }

    const QString & name() const {
        return m_name;
    }

private:
    void addChildren(QWebElement &parent);
    QString value() const;

private:
    QList<Fb2HeadItem*> m_list;
    QWebElement m_element;
    QString m_name;
    QString m_text;
    Fb2HeadItem * m_parent;
    QString m_id;
};

class Fb2HeadModel: public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit Fb2HeadModel(QWebView &view, QObject *parent = 0);
    virtual ~Fb2HeadModel();
    void select(const QModelIndex &index);
    void expand(QTreeView *view);

public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

protected:
    Fb2HeadItem * item(const QModelIndex &index) const;

private:
    QWebView & m_view;
    Fb2HeadItem * m_root;
};

#endif // FB2HEAD_H
