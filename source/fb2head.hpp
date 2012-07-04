#ifndef FB2HEAD_H
#define FB2HEAD_H

#include <QAbstractItemModel>
#include <QTreeView>
#include <QWebElement>
#include <QWebView>

#include "fb2xml.h"

class Fb2WebView;

class Fb2HeadItem: public QObject
{
    Q_OBJECT

    FB2_BEGIN_KEYLIST
        Auth,
        Cover,
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

    void setText(const QString &text);

    const QString & id() const {
        return m_id;
    }

    const QString & name() const {
        return m_name;
    }

    QString sub(const QString &key) const;

private:
    class HintHash : public QHash<QString, QString>
    {
    public:
        explicit HintHash();
    };

private:
    void addChildren(QWebElement &parent);
    QString value() const;
    QString hint() const;

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
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

protected:
    Fb2HeadItem * item(const QModelIndex &index) const;

private:
    QWebView & m_view;
    Fb2HeadItem * m_root;
};

class Fb2HeadView : public QTreeView
{
    Q_OBJECT

public:
    explicit Fb2HeadView(Fb2WebView &view, QWidget *parent = 0);

public slots:
    void editCurrent();
    void updateTree();

private slots:
    void activated(const QModelIndex &index);

private:
    Fb2WebView & m_view;
};

#endif // FB2HEAD_H
