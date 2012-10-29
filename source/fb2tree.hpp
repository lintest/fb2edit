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

class FbTextEdit;

class FbTreeItem;

class FbTreeModel;

typedef QList<FbTreeItem*> FbTreeList;

class FbTreeItem: public QObject
{
    Q_OBJECT

public:
    explicit FbTreeItem(QWebElement &element, FbTreeItem *parent = 0, int index = 0);

    virtual ~FbTreeItem();

    FbTreeItem * item(const QModelIndex &index) const;

    FbTreeItem * item(int row) const;

    FbTreeItem & operator=(const QWebElement &element) {
        m_element = element;
        return *this;
    }

    int index(FbTreeItem * child) const {
        return m_list.indexOf(child);
    }

    void insert(FbTreeItem * child, int row) {
        m_list.insert(row, child);
        child->m_parent = this;
    }

    FbTreeItem * takeAt(int row) {
        return m_list.takeAt(row);
    }

    bool hasChildren() {
        return m_list.size();
    }

    int count() const {
        return m_list.size();
    }

    FbTextElement element() const {
        return m_element;
    }

    FbTreeItem * parent() const {
        return m_parent;
    }

    const QString & name() const {
        return m_name;
    }

    QPoint pos() const {
        return m_element.geometry().topLeft();
    }

    FbTreeItem * content(int number) const;

    QString selector() const;

    QString text() const;

    void init();

private:
    QString title();

private:
    FbTreeList m_list;
    QWebElement m_element;
    QString m_name;
    QString m_text;
    QString m_body;
    FbTreeItem * m_parent;
    int m_number;
};

class FbTreeModel: public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit FbTreeModel(FbTextEdit &view, QObject *parent = 0);
    virtual ~FbTreeModel();
    QModelIndex index(FbTreeItem *item, int column = 0) const;
    QModelIndex index(const QString &location) const;
    FbTextEdit & view() { return m_view; }
    void selectText(const QModelIndex &index);
    QModelIndex move(const QModelIndex &index, int dx, int dy);
    QModelIndex append(const QModelIndex &parent, FbTextElement element);
    FbTreeItem * item(const QModelIndex &index) const;
    void update();

public:
    virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

private:
    void update(FbTreeItem &item);

private:
    FbTextEdit & m_view;
    FbTreeItem * m_root;
};

class FbTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit FbTreeView(FbTextEdit &view, QWidget *parent = 0);
    void initActions(QToolBar *toolbar);

private slots:
    void connectPage();
    void updateTree();
    void activated(const QModelIndex &index);
    void contextMenu(const QPoint &pos);
    void contentsChanged();
    void selectionChanged();
    void selectTree();

    void insertSection();
    void insertTitle();
    void insertAuthor();
    void insertEpigraph();
    void insertImage();
    void insertAnnot();
    void insertStanza();
    void insertDate();
    void deleteNode();

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();

    void treeDestroyed();
    void imgsDestroyed();

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    void append(const QModelIndex &parent, FbTextElement element);
    void moveCurrent(int dx, int dy);
    FbTreeModel * model() const;

private:
    FbTextEdit & m_view;
    QTimer m_timerSelect;
    QTimer m_timerUpdate;
    QAction
        *actionSection,
        *actionDelete,
        *actionTitle,
        *actionAuthor,
        *actionEpigraph,
        *actionStanza,
        *actionImage,
        *actionAnnot,
        *actionDate,
        *actionCut,
        *actionCopy,
        *actionPaste,
        *actionMoveUp,
        *actionMoveDown,
        *actionMoveLeft,
        *actionMoveRight;
};

class FbTreeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FbTreeWidget(FbTextEdit *view, QWidget* parent = 0);

protected:
    QToolBar * m_tool;
    FbTreeView * m_tree;
};

#endif // FB2TREE_H
