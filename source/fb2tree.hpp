#ifndef FB2TREE_H
#define FB2TREE_H

#include <QAbstractItemModel>
#include <QMenu>
#include <QTreeView>
#include <QTimer>
#include <QToolBar>

#include "fb2html.h"

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

class Fb2TextEdit;

class Fb2TreeItem;

class Fb2TreeModel;

typedef QList<Fb2TreeItem*> Fb2TreeList;

class Fb2TreeItem: public QObject
{
    Q_OBJECT

public:
    explicit Fb2TreeItem(QWebElement &element, Fb2TreeItem *parent = 0, int index = 0);

    virtual ~Fb2TreeItem();

    Fb2TreeItem * item(const QModelIndex &index) const;

    Fb2TreeItem * item(int row) const;

    Fb2TreeItem & operator=(const QWebElement &element) {
        m_element = element;
        return *this;
    }

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

    Fb2TextElement element() const {
        return m_element;
    }

    Fb2TreeItem * parent() const {
        return m_parent;
    }

    const QString & name() const {
        return m_name;
    }

    QPoint pos() const {
        return m_element.geometry().topLeft();
    }

    Fb2TreeItem * content(const Fb2TreeModel &model, int number) const;

    QString selector() const;

    QString text() const;

    void init();

private:
    QString title();

private:
    Fb2TreeList m_list;
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
    explicit Fb2TreeModel(Fb2TextEdit &view, QObject *parent = 0);
    virtual ~Fb2TreeModel();
    QModelIndex index(Fb2TreeItem *item, int column = 0) const;
    QModelIndex index(const QString &location) const;
    Fb2TextEdit & view() { return m_view; }
    void selectText(const QModelIndex &index);
    QModelIndex move(const QModelIndex &index, int dx, int dy);
    QModelIndex append(const QModelIndex &parent);
    Fb2TreeItem * item(const QModelIndex &index) const;
    void update();

public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

private:
    void update(Fb2TreeItem &item);

private:
    Fb2TextEdit & m_view;
    Fb2TreeItem * m_root;
};

class Fb2TreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit Fb2TreeView(Fb2TextEdit &view, QWidget *parent = 0);
    void initActions(QToolBar *toolbar);

public slots:
    void updateTree();

private slots:
    void activated(const QModelIndex &index);
    void contextMenu(const QPoint &pos);
    void contentsChanged();
    void selectionChanged();
    void selectTree();

    void insertNode();
    void deleteNode();

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    void moveCurrent(int dx, int dy);
    Fb2TreeModel * model();

private:
    Fb2TextEdit & m_view;
    QTimer m_timerSelect;
    QTimer m_timerUpdate;
    QMenu m_menu;
    QAction
        *actionCut,
        *actionCopy,
        *actionPaste;
};

class Fb2TreeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit Fb2TreeWidget(Fb2TextEdit &view, QWidget* parent = 0);

protected:
    QToolBar * m_tool;
    Fb2TreeView * m_tree;
};

#endif // FB2TREE_H
