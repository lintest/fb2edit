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

#include "fb2page.hpp"
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
    for (FbTreeItem * item: m_list) {
        delete item;
    }
}

void FbTreeItem::init()
{
    m_text = QString();
    m_name = m_element.tagName().toLower();
    if (m_name.left(3) == "fb:") m_name = m_name.mid(3);
    if (m_name == "title") {
        m_text = title();
        if (m_parent) m_parent->m_text += m_text += " ";
    } else if (m_name == "subtitle") {
        m_text = title();
    } else if (m_name == "body") {
        m_body = m_element.attribute("name");
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
            ++index;
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
    for (it = m_list.constBegin(); it != m_list.constEnd(); ++it) {
        if ((*it)->element() == element) return *it;
    }
    return nullptr;
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

bool FbTreeModel::hasChildren(const QModelIndex &parent) const
{
    FbTreeItem *owner = item(parent);
    return owner ? owner->hasChildren() : false;
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
    for (int i = last; i >= row; --i) {
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
    for (FbElementList::iterator it = list.begin(); it != list.end(); ++it) {
        FbTreeItem * child = nullptr;
        QWebElement element = *it;
        int count = owner.count();
        for (int i = pos; i < count; ++i) {
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
        ++pos;
    }

    int last = owner.count() - 1;
    if (pos <= last) {
        beginRemoveRows(index, pos, last);
        for (int i = last; i >= pos; --i) delete owner.takeAt(i);
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

QModelIndex FbTreeModel::append(const QModelIndex &parent, FbTextElement element)
{
    FbTreeItem * owner = item(parent);
    if (!owner || owner == m_root) return QModelIndex();

    int count = owner->count();
    int row = element.childIndex();
    if (row > count) row = count;
    if (row < 0) row = 0;

    FbTreeItem * child = new FbTreeItem(element);
    beginInsertRows(parent, row, row);
    owner->insert(child, row);
    endInsertRows();

    return createIndex(row, 0, (void*)child);
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

    connect(&m_view, SIGNAL(loadFinished(bool)), SLOT(updateTree()));
    connect(&m_view, SIGNAL(loadFinished(bool)), SLOT(connectPage()));
    connect(this, SIGNAL(activated(QModelIndex)), SLOT(activated(QModelIndex)));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(contextMenu(QPoint)));
    connectPage();

    m_timerSelect.setInterval(1000);
    m_timerSelect.setSingleShot(true);
    connect(&m_timerSelect, SIGNAL(timeout()), SLOT(selectTree()));

    m_timerUpdate.setInterval(1000);
    m_timerUpdate.setSingleShot(true);
    connect(&m_timerUpdate, SIGNAL(timeout()), SLOT(updateTree()));

    QMetaObject::invokeMethod(this, "updateTree", Qt::QueuedConnection);
}

void FbTreeView::connectPage()
{
    QWebPage *page = m_view.page();
    connect(page, SIGNAL(contentsChanged()), SLOT(contentsChanged()));
    connect(page, SIGNAL(selectionChanged()), SLOT(selectionChanged()));
    connect(page->undoStack(), SIGNAL(indexChanged(int)), SLOT(contentsChanged()));
}

void FbTreeView::initActions(QToolBar *toolbar)
{
    QAction * act;

    actionSection = act = new QAction(FbIcon("list-add"), tr("&Add section"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(Qt::Key_Insert);
    act->setPriority(QAction::LowPriority);
    connect(act, SIGNAL(triggered()), SLOT(insertSection()));
    toolbar->addAction(act);

    actionTitle = act = new QAction(tr("+ Title"), this);
    connect(act, SIGNAL(triggered()), SLOT(insertTitle()));

    actionEpigraph = act = new QAction(tr("+ Epigraph"), this);
    connect(act, SIGNAL(triggered()), SLOT(insertEpigraph()));

    actionImage = act = new QAction(tr("+ Image"), this);
    connect(act, SIGNAL(triggered()), SLOT(insertImage()));

    actionAnnot = act = new QAction(tr("+ Annotation"), this);
    connect(act, SIGNAL(triggered()), SLOT(insertAnnot()));

    actionStanza = act = new QAction(tr("+ Stanza"), this);
    connect(act, SIGNAL(triggered()), SLOT(insertStanza()));

    actionAuthor = act = new QAction(tr("+ Author"), this);
    connect(act, SIGNAL(triggered()), SLOT(insertAuthor()));

    actionText = act = new QAction(tr("+ Simple text"), this);
    connect(act, SIGNAL(triggered()), SLOT(insertText()));

    actionDate = act = new QAction(tr("+ Date"), this);
    connect(act, SIGNAL(triggered()), SLOT(insertDate()));

    actionDelete = act = new QAction(FbIcon("list-remove"), tr("&Delete"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(Qt::Key_Delete);
    act->setPriority(QAction::LowPriority);
    connect(act, SIGNAL(triggered()), SLOT(deleteNode()));
    toolbar->addAction(act);

    actionCut = act = new QAction(FbIcon("edit-cut"), tr("Cu&t"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Cut);
    act->setEnabled(false);

    actionCopy = act = new QAction(FbIcon("edit-copy"), tr("&Copy"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Copy);
    act->setEnabled(false);

    actionPaste = act = new QAction(FbIcon("edit-paste"), tr("&Paste"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Paste);

    toolbar->addSeparator();

    actionMoveUp = act = new QAction(FbIcon("go-up"), tr("&Up"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
    connect(act, SIGNAL(triggered()), SLOT(moveUp()));
    toolbar->addAction(act);

    actionMoveDown = act = new QAction(FbIcon("go-down"), tr("&Down"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
    connect(act, SIGNAL(triggered()), SLOT(moveDown()));
    toolbar->addAction(act);

    actionMoveLeft = act = new QAction(FbIcon("go-previous"), tr("&Left"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
    connect(act, SIGNAL(triggered()), SLOT(moveLeft()));
    toolbar->addAction(act);

    actionMoveRight = act = new QAction(FbIcon("go-next"), tr("&Right"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
    connect(act, SIGNAL(triggered()), SLOT(moveRight()));
    toolbar->addAction(act);
}

void FbTreeView::keyPressEvent(QKeyEvent *event)
{
    switch (event->modifiers()) {
        case Qt::NoModifier:
            switch (event->key()) {
                case Qt::Key_Insert: insertSection(); return;
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
    FbTreeModel * m = model();
    if (!m) return;

    FbTreeItem * i = m->item(currentIndex());
    if (!i) return;

    FbTextElement e = i->element();
    if (e.isTitle()) e = e.parent();
    if (e.isNull()) return;

    QMenu menu;
    menu.addAction(actionSection);

    QString tag = e.tagName();

    if (tag == "FB:BODY") {
        if (!e.hasChild("IMG")) menu.addAction(actionImage);
        if (!e.hasTitle()) menu.addAction(actionTitle);
        menu.addAction(actionEpigraph);
    }
    else
    if (tag == "FB:SECTION") {
        if (!e.hasTitle()) menu.addAction(actionTitle);
        menu.addAction(actionEpigraph);
        if (!e.hasChild("IMG")) menu.addAction(actionImage);
        if (!e.hasChild("FB:ANNOTATION")) menu.addAction(actionAnnot);
        menu.addAction(actionText);
    }
    else
    if (tag == "FB:POEM") {
        if (!e.hasTitle()) menu.addAction(actionTitle);
        menu.addAction(actionEpigraph);
        menu.addAction(actionStanza);
        menu.addAction(actionAuthor);
        if (!e.hasChild("date")) menu.addAction(actionDate);
    }
    else
    if (tag == "FB:STANZA") {
        if (!e.hasTitle()) menu.addAction(actionTitle);
    }
    else
    if (tag == "FB:EPIGRAPH") {
        menu.addAction(actionAuthor);
    }
    else
    if (tag == "FB:CITE") {
        menu.addAction(actionAuthor);
    }

    menu.addAction(actionDelete);
    menu.addSeparator();
    menu.addAction(actionCut);
    menu.addAction(actionCopy);
    menu.addAction(actionPaste);
    menu.addSeparator();
    menu.addAction(actionMoveUp);
    menu.addAction(actionMoveDown);
    menu.addAction(actionMoveLeft);
    menu.addAction(actionMoveRight);
    menu.exec(mapToGlobal(pos));
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

void FbTreeView::append(const QModelIndex &parent, FbTextElement element)
{
    FbTreeModel * m = model();
    if (!m) return;

    QModelIndex current = currentIndex();
    QModelIndex index = m->append(parent, element);
    if (!index.isValid()) return;

    setCurrentIndex(index);
    emit QTreeView::currentChanged(index, current);
    emit QTreeView::activated(index);
    scrollTo(index);
}

void FbTreeView::insertSection()
{
    FbTreeModel * m = model();
    if (!m) return;

    QModelIndex index = currentIndex();
    FbTreeItem * item = m->item(index);
    if (!item) return;

    FbTextElement element = item->element();
    while (!element.isNull()) {
        if (element.isBody() || element.isSection()) {
            element = m_view.page()->appendSection(element);
            append(index, element);
            break;
        }
        element = element.parent();
        index = m->parent(index);
    }
}

void FbTreeView::insertTitle()
{
    FbTreeModel * m = model();
    if (!m) return;

    QModelIndex index = currentIndex();
    FbTreeItem * item = m->item(index);
    if (!item) return;

    FbTextElement element = item->element();
    if (element.hasTitle()) return;
    element = m_view.page()->appendTitle(element);
    append(index, element);
}

void FbTreeView::insertText()
{
    FbTreeModel * m = model();
    if (!m) return;

    QModelIndex index = currentIndex();
    FbTreeItem * item = m->item(index);
    if (!item) return;

    FbTextElement element = item->element();
    if (!element.isSection()) return;
    m_view.page()->appendText(element).select();
}

void FbTreeView::insertAuthor()
{
}

void FbTreeView::insertEpigraph()
{
}

void FbTreeView::insertImage()
{
}

void FbTreeView::insertAnnot()
{
}

void FbTreeView::insertStanza()
{
}

void FbTreeView::insertDate()
{
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

FbTreeModel * FbTreeView::model() const
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

FbTreeWidget::FbTreeWidget(FbTextEdit *view, QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout * layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tree = new FbTreeView(*view, this);
    layout->addWidget(m_tree);

    m_tool = new QToolBar(this);
    layout->addWidget(m_tool);

    m_tree->initActions(m_tool);
}
