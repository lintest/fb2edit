#ifndef FB2TREE_HPP
#define FB2TREE_HPP

#include <QAbstractItemModel>
#include <QTreeView>
#include <QTimer>
#include <QToolBar>

#include "fb2html.h"

class Fb2WebView;

class Fb2TreeModel;

class Fb2TreeItem: public QObject
{
    Q_OBJECT

public:
    explicit Fb2TreeItem(QWebElement &element, Fb2TreeItem *parent = 0, int index = 0);

    virtual ~Fb2TreeItem();

    Fb2TreeItem * item(const QModelIndex &index) const;

    Fb2TreeItem * item(int row) const;

    int index(Fb2TreeItem * child) const {
        return m_list.indexOf(child);
    }

    void insert(Fb2TreeItem * child, int row) {
        m_list.insert(row, child);
        child->m_parent = this;
    }

    Fb2TreeItem * takeAt(int row) {
        return m_list.takeAt(row);
    }

    int count() const {
        return m_list.size();
    }

    Fb2WebElement element() const {
        return m_element;
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

    Fb2TreeItem * content(const Fb2TreeModel &model, int number, QModelIndex &index) const;

private:
    QString static title(const QWebElement &element);
    void addChildren(QWebElement &parent, bool direct = true, bool header = false);

private:
    QList<Fb2TreeItem*> m_list;
    QWebElement m_element;
    QString m_name;
    QString m_text;
    QString m_body;
    Fb2TreeItem * m_parent;
    int m_number;
};

class Fb2TreeModel: public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit Fb2TreeModel(Fb2WebView &view, QObject *parent = 0);
    virtual ~Fb2TreeModel();
    QModelIndex index(const QString &location) const;
    Fb2WebView & view() { return m_view; }
    void selectText(const QModelIndex &index);
    QModelIndex move(const QModelIndex &index, int dx, int dy);

public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

protected:
    Fb2TreeItem * item(const QModelIndex &index) const;

private:
    Fb2WebView & m_view;
    Fb2TreeItem * m_root;
};

class Fb2TreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit Fb2TreeView(Fb2WebView &view, QWidget *parent = 0);
    void initToolbar(QToolBar *toolbar);

public slots:
    void updateTree();

private slots:
    void activated(const QModelIndex &index);
    void contentsChanged();
    void selectionChanged();
    void selectTree();

    void insertNode();
    void deleteNode();

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();

private:
    void moveCurrent(int dx, int dy);
    Fb2TreeModel * model();

private:
    Fb2WebView & m_view;
    QTimer m_timerSelect;
    QTimer m_timerUpdate;
};

class Fb2TreeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit Fb2TreeWidget(Fb2WebView &view, QWidget* parent = 0);

protected:
    QToolBar * m_tool;
    Fb2TreeView * m_tree;
};

#endif // FB2TREE_HPP
