#include "fb2tree.hpp"

#include <QtDebug>
#include <QAction>
#include <QApplication>
#include <QVBoxLayout>
#include <QUndoStack>
#include <QWebFrame>
#include <QWebPage>
#include <QTreeView>
#include <QUrl>

#include "fb2text.hpp"
#include "fb2html.h"
#include "fb2utils.h"

//---------------------------------------------------------------------------
//  FbTreeItem
//---------------------------------------------------------------------------

FbTreeItem::FbTreeItem(QWebElement &element, FbTreeItem *parent, int number)
    : QObject(parent)
    , m_element(element)
    , m_parent(parent)
    , m_number(number)
{
    init();
}

FbTreeItem::~FbTreeItem()
{
    foreach (FbTreeItem * item, m_list) {
        delete item;
    }
}

void FbTreeItem::init()
{
    m_text = QString();
    m_name = m_element.tagName().toLower();
    QString style = m_element.attribute("class").toLower();
    if (m_name == "div") {
        if (style == "title") {
            m_text = title();
            if (m_parent) m_parent->m_text += m_text += " ";
        } else if (style == "subtitle") {
            m_text = title();
        } else if (style == "body") {
            m_body = m_element.attribute("fb2_name");
        }
        if (!style.isEmpty()) m_name = style;
    } else if (m_name == "img") {
        m_name = "image";
        QUrl url = m_element.attribute("src");
        m_text = url.fragment();
    }
}

QString FbTreeItem::title()
{
    return m_element.toPlainText().left(255).simplified();
}

FbTreeItem * FbTreeItem::item(const QModelIndex &index) const
{
    int row = index.row();
    if (row < 0 || row >= m_list.size()) return NULL;
    return m_list[row];
}

FbTreeItem * FbTreeItem::item(int row) const
{
    if (row < 0 || row >= m_list.size()) return NULL;
    return m_list[row];
}

QString FbTreeItem::text() const
{
    QString name = m_name;
    if (!m_body.isEmpty()) name += " name=" + m_body;
    return QString("<%1> %2").arg(name).arg(m_text);
}

QString FbTreeItem::selector() const
{
    QString text = "";
    QString selector = ".get(0)";
    QWebElement element = m_element;
    QWebElement parent = element.parent();
    while (!parent.isNull()) {
        text.prepend(element.tagName()).prepend("/");
        QWebElement child = parent.firstChild();
        int index = -1;
        while (!child.isNull()) {
            index++;
            if (child == element) break;
            child = child.nextSibling();
        }
        if (index == -1) return QString();
        selector.prepend(QString(".children().eq(%1)").arg(index));
        element = parent;
        parent = element.parent();
    }
    return selector.prepend("$('html')");
}

FbTreeItem * FbTreeItem::content(int number) const
{
    FbTextElement element = m_element.firstChild();
    while (number-- > 0) element = element.nextSibling();
    FbTreeList::const_iterator it;
    for (it = m_list.constBegin(); it != m_list.constEnd(); it++) {
        if ((*it)->element() == element) return *it;
    }
    return 0;
}

//---------------------------------------------------------------------------
//  FbTreeModel
//---------------------------------------------------------------------------

FbTreeModel::FbTreeModel(FbTextEdit &view, QObject *parent)
    : QAbstractItemModel(parent)
    , m_view(view)
    , m_root(NULL)
{
    QWebElement doc = view.page()->mainFrame()->documentElement();
    QWebElement body = doc.findFirst("body");
    if (body.isNull()) return;
    m_root = new FbTreeItem(body);
}

FbTreeModel::~FbTreeModel()
{
    if (m_root) delete m_root;
}

FbTreeItem * FbTreeModel::item(const QModelIndex &index) const
{
    if (index.isValid()) {
        return static_cast<FbTreeItem*>(index.internalPointer());
    } else {
        return m_root;
    }
}

int FbTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex FbTreeModel::index(FbTreeItem *item, int column) const
{
    FbTreeItem *parent = item->parent();
    return parent ? createIndex(parent->index(item), column, (void*)item) : QModelIndex();
}

QModelIndex FbTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!m_root || row < 0 || column < 0) return QModelIndex();
    if (FbTreeItem *owner = item(parent)) {
        if (FbTreeItem *child = owner->item(row)) {
            return createIndex(row, column, (void*)child);
        }
    }
    return QModelIndex();
}

QModelIndex FbTreeModel::parent(const QModelIndex &child) const
{
    if (FbTreeItem * node = static_cast<FbTreeItem*>(child.internalPointer())) {
        if (FbTreeItem * parent = node->parent()) {
            if (FbTreeItem * owner = parent->parent()) {
                return createIndex(owner->index(parent), 0, (void*)parent);
            }
        }
    }
    return QModelIndex();
}

int FbTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) return 0;
    FbTreeItem *owner = item(parent);
    return owner ? owner->count() : 0;
}

QVariant FbTreeModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();
    FbTreeItem * i = item(index);
    return i ? i->text() : QVariant();
}

void FbTreeModel::selectText(const QModelIndex &index)
{
    if (FbTreeItem *node = item(index)) {
        node->element().select();
    }
}

QModelIndex FbTreeModel::index(const QString &location) const
{
    QModelIndex result;
    FbTreeItem * parent = m_root;
    QStringList list = location.split(",");
    QStringListIterator iterator(list);
    while (parent && iterator.hasNext()) {
        QString str = iterator.next();
        if (str.left(5) == "HTML=") continue;
        int key = str.mid(str.indexOf("=")+1).toInt();
        FbTreeItem * child = parent->content(key);
        if (child) result = index(child);
        parent = child;
    }
    return result;
}

QModelIndex FbTreeModel::move(const QModelIndex &index, int dx, int dy)
{
    FbTreeItem *child = item(index);
    if (!child) return QModelIndex();

    FbTreeItem *owner = child->parent();
    if (!owner) return QModelIndex();

    int from = index.row();
    QModelIndex parent = this->parent(index);
    QModelIndex result;

    switch (dx) {
        case -1: {
            if (!owner || owner == m_root) return QModelIndex();
            if (!child->element().isSection()) return QModelIndex();
            if (!owner->element().isSection()) return QModelIndex();

            QModelIndex target = this->parent(parent);
            int to = parent.row() + 1;
            result = createIndex(to, 0, (void*)child);

            beginMoveRows(parent, from, from, target, to);
            owner->takeAt(from);
            owner->parent()->insert(child, to);
            endMoveRows();

            QUndoCommand * command = new FbMoveLeftCmd(child->element());
            m_view.page()->push(command, tr("Move section"));
        } break;

        case +1: {
            if (from == 0) return QModelIndex();
            FbTreeItem * brother = owner->item(from - 1);
            if (!child->element().isSection()) return QModelIndex();
            if (!brother->element().isSection()) return QModelIndex();

            QModelIndex target = createIndex(from - 1, 0, (void*)brother);
            int to = rowCount(target);
            result = createIndex(to, 0, (void*)child);

            beginMoveRows(parent, from, from, target, to);
            owner->takeAt(from);
            brother->insert(child, to);
            endMoveRows();

            QUndoCommand * command = new FbMoveRightCmd(child->element());
            m_view.page()->push(command, tr("Move section"));
        } break;

        default: {
            int to = from + dy;
            if (to < 0 || rowCount(parent) <= to) return QModelIndex();
            result = createIndex(to, 0, (void*)child);

            if (dy > 0) {
                to = index.row();
                from = to + dy;
            }

            FbTreeItem * child = owner->item(to);
            FbTreeItem * brother = owner->item(from);

            QString n = child->name();
            bool ok = (n == "body" || n == "section") && n == brother->name();
            if (!ok) return QModelIndex();

            beginMoveRows(parent, from, from, parent, to);
            brother = owner->takeAt(from);
            owner->insert(brother, to);
            endMoveRows();

            QUndoCommand * command = new FbMoveUpCmd(brother->element());
            m_view.page()->push(command, tr("Move section"));
        } break;
    }
    return result;
}

bool FbTreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || count <= 0 || row + count > rowCount(parent)) return false;
    FbTreeItem * owner = item(parent);
    if (!owner) return false;
    int last = row + count - 1;
    beginRemoveRows(parent, row, last);
    for (int i = last; i >= row; i--) {
        if (FbTreeItem * child = owner->takeAt(i)) {
            QUndoCommand * command = new FbDeleteCmd(child->element());
            m_view.page()->push(command, "Delete element");
            delete child;
        }
    }
    endRemoveRows();
    return true;
}

void FbTreeModel::update(FbTreeItem &owner)
{
    owner.init();
    FbElementList list;
    owner.element().getChildren(list);

    int pos = 0;
    QModelIndex index = this->index(&owner);
    for (FbElementList::iterator it = list.begin(); it != list.end(); it++) {
        FbTreeItem * child = 0;
        QWebElement element = *it;
        int count = owner.count();
        for (int i = pos; i < count; i++) {
            if (owner.item(i)->element() == element) {
                child = owner.item(i);
                if (i > pos) {
                    beginMoveRows(index, i, i, index, pos);
                    owner.insert(owner.takeAt(i), pos);
                    endMoveRows();
                    break;
                }
            }
        }
        if (child) {
            QString old = child->text();
            update(*child);
            if (old != child->text()) {
                QModelIndex i = this->index(child);
                emit dataChanged(i, i);
            }
        } else {
            FbTreeItem * child = new FbTreeItem(element);
            beginInsertRows(index, pos, pos);
            owner.insert(child, pos);
            endInsertRows();
            update(*child);
        }
        pos++;
    }

    int last = owner.count() - 1;
    if (pos <= last) {
        beginRemoveRows(index, pos, last);
        for (int i = last; i >= pos; i--) delete owner.takeAt(i);
        endRemoveRows();
    }
}

void FbTreeModel::update()
{
    QWebElement doc = m_view.page()->mainFrame()->documentElement();
    QWebElement body = doc.findFirst("body");
    if (m_root) {
        if (m_root->element() != body) *m_root = body;
        update(*m_root);
    } else {
        if (!body.isNull()) {
            m_root = new FbTreeItem(body);
            update(*m_root);
        }
    }
}

//---------------------------------------------------------------------------
//  FbTreeView
//---------------------------------------------------------------------------

FbTreeView::FbTreeView(FbTextEdit &view, QWidget *parent)
    : QTreeView(parent)
    , m_view(view)
{
    setHeaderHidden(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(activated(QModelIndex)), SLOT(activated(QModelIndex)));
    connect(m_view.page(), SIGNAL(loadFinished(bool)), SLOT(updateTree()));
    connect(m_view.page(), SIGNAL(contentsChanged()), SLOT(contentsChanged()));
    connect(m_view.page(), SIGNAL(selectionChanged()), SLOT(selectionChanged()));
    connect(m_view.page()->undoStack(), SIGNAL(indexChanged(int)), SLOT(contentsChanged()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(contextMenu(QPoint)));

    m_timerSelect.setInterval(1000);
    m_timerSelect.setSingleShot(true);
    connect(&m_timerSelect, SIGNAL(timeout()), SLOT(selectTree()));

    m_timerUpdate.setInterval(1000);
    m_timerUpdate.setSingleShot(true);
    connect(&m_timerUpdate, SIGNAL(timeout()), SLOT(updateTree()));

    QMetaObject::invokeMethod(this, "updateTree", Qt::QueuedConnection);
}

void FbTreeView::initActions(QToolBar *toolbar)
{
    QAction * act;

    act = new QAction(FbIcon("list-add"), tr("&Insert"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(Qt::Key_Insert);
    act->setPriority(QAction::LowPriority);
    connect(act, SIGNAL(triggered()), SLOT(insertNode()));
    toolbar->addAction(act);
    m_menu.addAction(act);

    act = new QAction(FbIcon("list-remove"), tr("&Delete"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(Qt::Key_Delete);
    act->setPriority(QAction::LowPriority);
    connect(act, SIGNAL(triggered()), SLOT(deleteNode()));
    toolbar->addAction(act);
    m_menu.addAction(act);

    m_menu.addSeparator();

    actionCut = act = new QAction(FbIcon("edit-cut"), tr("Cu&t"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Cut);
    act->setEnabled(false);
    m_menu.addAction(act);

    actionCopy = act = new QAction(FbIcon("edit-copy"), tr("&Copy"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Copy);
    act->setEnabled(false);
    m_menu.addAction(act);

    actionPaste = act = new QAction(FbIcon("edit-paste"), tr("&Paste"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Paste);
    m_menu.addAction(act);

    toolbar->addSeparator();
    m_menu.addSeparator();

    act = new QAction(FbIcon("go-up"), tr("&Up"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
    connect(act, SIGNAL(triggered()), SLOT(moveUp()));
    toolbar->addAction(act);
    m_menu.addAction(act);

    act = new QAction(FbIcon("go-down"), tr("&Down"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
    connect(act, SIGNAL(triggered()), SLOT(moveDown()));
    toolbar->addAction(act);
    m_menu.addAction(act);

    act = new QAction(FbIcon("go-previous"), tr("&Left"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
    connect(act, SIGNAL(triggered()), SLOT(moveLeft()));
    toolbar->addAction(act);
    m_menu.addAction(act);

    act = new QAction(FbIcon("go-next"), tr("&Right"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
    connect(act, SIGNAL(triggered()), SLOT(moveRight()));
    toolbar->addAction(act);
    m_menu.addAction(act);
}

void FbTreeView::keyPressEvent(QKeyEvent *event)
{
    switch (event->modifiers()) {
        case Qt::NoModifier:
            switch (event->key()) {
                case Qt::Key_Insert: insertNode(); return;
                case Qt::Key_Delete: deleteNode(); return;
            }
            break;
    case Qt::ControlModifier:
        switch (event->key()) {
            case Qt::Key_Left  : moveCurrent(-1, 0); return;
            case Qt::Key_Right : moveCurrent(+1, 0); return;
            case Qt::Key_Up    : moveCurrent(0, -1); return;
            case Qt::Key_Down  : moveCurrent(0, +1); return;
        }
        break;
    }
    QTreeView::keyPressEvent(event);
}

void FbTreeView::contextMenu(const QPoint &pos)
{
    m_menu.exec(mapToGlobal(pos));
}

void FbTreeView::selectionChanged()
{
    m_timerSelect.start();
}

void FbTreeView::contentsChanged()
{
    m_timerUpdate.start();
}

void FbTreeView::activated(const QModelIndex &index)
{
    if (qApp->focusWidget() == &m_view) return;
    if (FbTreeModel * m = model()) {
        m->selectText(index);
    }
}

void FbTreeView::selectTree()
{
    if (qApp->focusWidget() == this) return;
    if (FbTreeModel * m = model()) {
        QString location = m->view().page()->location();
        QModelIndex index = m->index(location);
        if (!index.isValid()) return;
        setCurrentIndex(index);
        scrollTo(index);
    }
}

void FbTreeView::updateTree()
{
    if (FbTreeModel * m = model()) {
        m->update();
    } else {
        m = new FbTreeModel(m_view, this);
        m->update();
        setModel(m);
    }
    selectTree();
}

QModelIndex FbTreeModel::append(const QModelIndex &parent, FbTextElement element)
{
    FbTreeItem * owner = item(parent);
    if (!owner || owner == m_root) return QModelIndex();
    int row = owner->count();
    FbTreeItem * child = new FbTreeItem(element);
    beginInsertRows(parent, row, row);
    owner->insert(child, row);
    endInsertRows();
    return createIndex(row, 0, (void*)child);
}

void FbTreeView::insertNode()
{
    if (FbTreeModel * m = model()) {
        QModelIndex index = currentIndex();
        FbTreeItem * item = m->item(index);
        if (!item) return;

        FbTextElement element = item->element();
        while (!element.isNull()) {
            if (element.isSection() || element.isBody()) {
                m_view.page()->appendSection(element);
                QModelIndex result = m->append(index, element.lastChild());
                if (!result.isValid()) return;
                setCurrentIndex(result);
                emit QTreeView::currentChanged(result, index);
                emit QTreeView::activated(result);
                scrollTo(result);
                break;
            }
            element = element.parent();
            index = m->parent(index);
        }
    }
}

void FbTreeView::deleteNode()
{
    if (FbTreeModel * m = model()) {
        QModelIndex index = currentIndex();
        QModelIndex parent = m->parent(index);

        QModelIndex result = parent;
        int row = index.row();
        int last = m->rowCount(result) - 1;
        if (last > 0)  {
            if (row >= last) row = last;
            result = m->index(row, 0, parent);
        }
        emit currentChanged(result, index);
        emit QTreeView::activated(result);
        setCurrentIndex(result);
        m->removeRow(row, parent);
    }
}

FbTreeModel * FbTreeView::model()
{
    return qobject_cast<FbTreeModel*>(QTreeView::model());
}

void FbTreeView::moveCurrent(int dx, int dy)
{
    if (FbTreeModel * m = model()) {
        QModelIndex index = currentIndex();
        QModelIndex result = m->move(index, dx, dy);
        if (result.isValid()) {
            setCurrentIndex(result);
            emit currentChanged(result, index);
            emit QTreeView::activated(result);
            scrollTo(result);
        }
    }
}

void FbTreeView::moveUp()
{
    moveCurrent(0, -1);
}

void FbTreeView::moveDown()
{
    moveCurrent(0, +1);
}

void FbTreeView::moveLeft()
{
    moveCurrent(-1, 0);
}

void FbTreeView::moveRight()
{
    moveCurrent(+1, 0);
}

//---------------------------------------------------------------------------
//  FbTreeWidget
//---------------------------------------------------------------------------

FbTreeWidget::FbTreeWidget(FbTextEdit &view, QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout * layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setObjectName(QString::fromUtf8("verticalLayout"));

    m_tree = new FbTreeView(view, this);
    layout->addWidget(m_tree);

    m_tool = new QToolBar(this);
    layout->addWidget(m_tool);

    m_tree->initActions(m_tool);
}
