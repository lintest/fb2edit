#include "fb2tree.hpp"

#include <QtDebug>
#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QVBoxLayout>
#include <QWebFrame>
#include <QWebPage>
#include <QTreeView>
#include <QUrl>

#include "fb2text.hpp"
#include "fb2html.h"
#include "fb2utils.h"

//---------------------------------------------------------------------------
//  Fb2TreeItem
//---------------------------------------------------------------------------

Fb2TreeItem::Fb2TreeItem(QWebElement &element, Fb2TreeItem *parent, int number)
    : QObject(parent)
    , m_element(element)
    , m_parent(parent)
    , m_number(number)
{
    init();
}

Fb2TreeItem::~Fb2TreeItem()
{
    foreach (Fb2TreeItem * item, m_list) {
        delete item;
    }
}

void Fb2TreeItem::init()
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

QString Fb2TreeItem::title()
{
    return m_element.toPlainText().left(255).simplified();
}

Fb2TreeItem * Fb2TreeItem::item(const QModelIndex &index) const
{
    int row = index.row();
    if (row < 0 || row >= m_list.size()) return NULL;
    return m_list[row];
}

Fb2TreeItem * Fb2TreeItem::item(int row) const
{
    if (row < 0 || row >= m_list.size()) return NULL;
    return m_list[row];
}

QString Fb2TreeItem::text() const
{
    QString name = m_name;
    if (!m_body.isEmpty()) name += " name=" + m_body;
    return QString("<%1> %2").arg(name).arg(m_text);
}

QString Fb2TreeItem::selector() const
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

Fb2TreeItem * Fb2TreeItem::content(const Fb2TreeModel &model, int number) const
{
    Fb2TextElement element = m_element.firstChild();
    while (number-- > 0) element = element.nextSibling();
    Fb2TreeList::const_iterator it;
    for (it = m_list.constBegin(); it != m_list.constEnd(); it++) {
        if ((*it)->element() == element) return *it;
    }
    return 0;
}

//---------------------------------------------------------------------------
//  Fb2TreeModel
//---------------------------------------------------------------------------

Fb2TreeModel::Fb2TreeModel(Fb2TextEdit &view, QObject *parent)
    : QAbstractItemModel(parent)
    , m_view(view)
    , m_root(NULL)
{
    QWebElement doc = view.page()->mainFrame()->documentElement();
    QWebElement body = doc.findFirst("body");
    if (body.isNull()) return;
    m_root = new Fb2TreeItem(body);
}

Fb2TreeModel::~Fb2TreeModel()
{
    if (m_root) delete m_root;
}

Fb2TreeItem * Fb2TreeModel::item(const QModelIndex &index) const
{
    if (index.isValid()) {
        return static_cast<Fb2TreeItem*>(index.internalPointer());
    } else {
        return m_root;
    }
}

int Fb2TreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex Fb2TreeModel::index(Fb2TreeItem *item, int column) const
{
    Fb2TreeItem *parent = item->parent();
    return parent ? createIndex(parent->index(item), column, (void*)item) : QModelIndex();
}

QModelIndex Fb2TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!m_root || row < 0 || column < 0) return QModelIndex();
    if (Fb2TreeItem *owner = item(parent)) {
        if (Fb2TreeItem *child = owner->item(row)) {
            return createIndex(row, column, (void*)child);
        }
    }
    return QModelIndex();
}

QModelIndex Fb2TreeModel::parent(const QModelIndex &child) const
{
    if (Fb2TreeItem * node = static_cast<Fb2TreeItem*>(child.internalPointer())) {
        if (Fb2TreeItem * parent = node->parent()) {
            if (Fb2TreeItem * owner = parent->parent()) {
                return createIndex(owner->index(parent), 0, (void*)parent);
            }
        }
    }
    return QModelIndex();
}

int Fb2TreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) return 0;
    Fb2TreeItem *owner = item(parent);
    return owner ? owner->count() : 0;
}

QVariant Fb2TreeModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();
    Fb2TreeItem * i = item(index);
    return i ? i->text() : QVariant();
}

void Fb2TreeModel::selectText(const QModelIndex &index)
{
    if (Fb2TreeItem *node = item(index)) {
        node->element().select();
    }
}

QModelIndex Fb2TreeModel::index(const QString &location) const
{
    QModelIndex result;
    Fb2TreeItem * parent = m_root;
    QStringList list = location.split(",");
    QStringListIterator iterator(list);
    while (parent && iterator.hasNext()) {
        QString str = iterator.next();
        if (str.left(5) == "HTML=") continue;
        int key = str.mid(str.indexOf("=")+1).toInt();
        Fb2TreeItem * child = parent->content(*this, key);
        if (child) result = index(child);
        parent = child;
    }
    return result;
}

QModelIndex Fb2TreeModel::move(const QModelIndex &index, int dx, int dy)
{
    Fb2TreeItem *child = item(index);
    if (!child) return QModelIndex();

    Fb2TreeItem *owner = child->parent();
    if (!owner) return QModelIndex();

    int from = index.row();
    QModelIndex parent = this->parent(index);
    QModelIndex result;

    switch (dx) {
        case -1: {
            if (!owner || owner == m_root) return QModelIndex();
            if (child->name() != "section") return QModelIndex();
            if (owner->name() != "section") return QModelIndex();
            QModelIndex target = this->parent(parent);
            int to = parent.row() + 1;
            result = createIndex(to, 0, (void*)child);
            beginMoveRows(parent, from, from, target, to);
            QWebElement element = child->element().takeFromDocument();
            owner->element().appendOutside(element);
            owner->takeAt(from);
            owner->parent()->insert(child, to);
            endMoveRows();
        } break;

        case +1: {
            if (from == 0) return QModelIndex();
            Fb2TreeItem * brother = owner->item(from - 1);
            if (child->name() != "section") return QModelIndex();
            if (brother->name() != "section") return QModelIndex();
            QModelIndex target = createIndex(from - 1, 0, (void*)brother);
            int to = rowCount(target);
            result = createIndex(to, 0, (void*)child);
            beginMoveRows(parent, from, from, target, to);
            QWebElement element = child->element().takeFromDocument();
            brother->element().appendInside(element);
            owner->takeAt(from);
            brother->insert(child, to);
            endMoveRows();
        } break;

        default: {
            int to = from + dy;
            if (to < 0 || rowCount(parent) <= to) return QModelIndex();
            result = createIndex(to, 0, (void*)child);

            if (dy > 0) {
                to = index.row();
                from = to + dy;
            }

            Fb2TreeItem * child = owner->item(to);
            Fb2TreeItem * brother = owner->item(from);

            QString n = child->name();
            bool ok = (n == "body" || n == "section") && n == brother->name();
            if (!ok) return QModelIndex();

            beginMoveRows(parent, from, from, parent, to);
            brother = owner->takeAt(from);
            owner->insert(brother, to);
            QWebElement element = child->element().takeFromDocument();
            brother->element().appendOutside(element);
            endMoveRows();
        } break;
    }
    return result;
}

bool Fb2TreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || count <= 0 || row + count > rowCount(parent)) return false;
    Fb2TreeItem * owner = item(parent);
    if (!owner) return false;
    int last = row + count - 1;
    beginRemoveRows(parent, row, last);
    for (int i = last; i >= row; i--) {
        if (Fb2TreeItem * child = owner->takeAt(i)) {
            Fb2TextPage & page = *m_view.page();
            page.undoStack()->beginMacro("Delete element");
            page.undoStack()->push(new Fb2DeleteCmd(page, child->element()));
            page.undoStack()->endMacro();
            delete child;
        }
    }
    endRemoveRows();
    return true;
}

void Fb2TreeModel::update(Fb2TreeItem &owner)
{
    owner.init();
    Fb2ElementList list;
    owner.element().getChildren(list);

    int pos = 0;
    QModelIndex index = this->index(&owner);
    for (Fb2ElementList::iterator it = list.begin(); it != list.end(); it++) {
        Fb2TreeItem * child = 0;
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
            Fb2TreeItem * child = new Fb2TreeItem(element);
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

void Fb2TreeModel::update()
{
    QWebElement doc = m_view.page()->mainFrame()->documentElement();
    QWebElement body = doc.findFirst("body");
    if (m_root) {
        if (m_root->element() != body) *m_root = body;
        update(*m_root);
    } else {
        if (!body.isNull()) {
            m_root = new Fb2TreeItem(body);
            update(*m_root);
        }
    }
}

//---------------------------------------------------------------------------
//  Fb2TreeView
//---------------------------------------------------------------------------

Fb2TreeView::Fb2TreeView(Fb2TextEdit &view, QWidget *parent)
    : QTreeView(parent)
    , m_view(view)
{
    setHeaderHidden(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(activated(QModelIndex)), SLOT(activated(QModelIndex)));
    connect(m_view.page(), SIGNAL(loadFinished(bool)), SLOT(updateTree()));
    connect(m_view.page(), SIGNAL(contentsChanged()), SLOT(contentsChanged()));
    connect(m_view.page(), SIGNAL(selectionChanged()), SLOT(selectionChanged()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(contextMenu(QPoint)));

    m_timerSelect.setInterval(1000);
    m_timerSelect.setSingleShot(true);
    connect(&m_timerSelect, SIGNAL(timeout()), SLOT(selectTree()));

    m_timerUpdate.setInterval(1000);
    m_timerUpdate.setSingleShot(true);
    connect(&m_timerUpdate, SIGNAL(timeout()), SLOT(updateTree()));

    QMetaObject::invokeMethod(this, "updateTree", Qt::QueuedConnection);
}

void Fb2TreeView::initActions(QToolBar *toolbar)
{
    QAction * act;

    act = new QAction(FB2::icon("list-add"), tr("&Insert"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(Qt::Key_Insert);
    act->setPriority(QAction::LowPriority);
    connect(act, SIGNAL(triggered()), SLOT(insertNode()));
    toolbar->addAction(act);
    m_menu.addAction(act);

    act = new QAction(FB2::icon("list-remove"), tr("&Delete"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(Qt::Key_Delete);
    act->setPriority(QAction::LowPriority);
    connect(act, SIGNAL(triggered()), SLOT(deleteNode()));
    toolbar->addAction(act);
    m_menu.addAction(act);

    m_menu.addSeparator();

    actionCut = act = new QAction(FB2::icon("edit-cut"), tr("Cu&t"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Cut);
    act->setEnabled(false);
    m_menu.addAction(act);

    actionCopy = act = new QAction(FB2::icon("edit-copy"), tr("&Copy"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Copy);
    act->setEnabled(false);
    m_menu.addAction(act);

    actionPaste = act = new QAction(FB2::icon("edit-paste"), tr("&Paste"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Paste);
    m_menu.addAction(act);

    toolbar->addSeparator();
    m_menu.addSeparator();

    act = new QAction(FB2::icon("go-up"), tr("&Up"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Up));
    connect(act, SIGNAL(triggered()), SLOT(moveUp()));
    toolbar->addAction(act);
    m_menu.addAction(act);

    act = new QAction(FB2::icon("go-down"), tr("&Down"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Down));
    connect(act, SIGNAL(triggered()), SLOT(moveDown()));
    toolbar->addAction(act);
    m_menu.addAction(act);

    act = new QAction(FB2::icon("go-previous"), tr("&Left"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Left));
    connect(act, SIGNAL(triggered()), SLOT(moveLeft()));
    toolbar->addAction(act);
    m_menu.addAction(act);

    act = new QAction(FB2::icon("go-next"), tr("&Right"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Right));
    connect(act, SIGNAL(triggered()), SLOT(moveRight()));
    toolbar->addAction(act);
    m_menu.addAction(act);
}

void Fb2TreeView::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::NoModifier) {
        switch (event->key()) {
            case Qt::Key_Insert: insertNode(); return;
            case Qt::Key_Delete: deleteNode(); return;
        }
    }
    QTreeView::keyPressEvent(event);
}

void Fb2TreeView::contextMenu(const QPoint &pos)
{
    m_menu.exec(QCursor::pos());
}

void Fb2TreeView::selectionChanged()
{
    m_timerSelect.start();
}

void Fb2TreeView::contentsChanged()
{
    m_timerUpdate.start();
}

void Fb2TreeView::activated(const QModelIndex &index)
{
    if (qApp->focusWidget() == &m_view) return;
    if (Fb2TreeModel * m = model()) {
        m->selectText(index);
    }
}

void Fb2TreeView::selectTree()
{
    if (qApp->focusWidget() == this) return;
    if (Fb2TreeModel * m = model()) {
        QString location = m->view().page()->location();
        QModelIndex index = m->index(location);
        if (!index.isValid()) return;
        setCurrentIndex(index);
        scrollTo(index);
    }
}

void Fb2TreeView::updateTree()
{
    if (Fb2TreeModel * m = model()) {
        m->update();
    } else {
        m = new Fb2TreeModel(m_view, this);
        m->update();
        setModel(m);
    }
    selectTree();
}

QModelIndex Fb2TreeModel::append(const QModelIndex &parent, Fb2TextElement element)
{
    Fb2TreeItem * owner = item(parent);
    if (!owner || owner == m_root) return QModelIndex();
    int row = owner->count();
    Fb2TreeItem * child = new Fb2TreeItem(element);
    beginInsertRows(parent, row, row);
    owner->insert(child, row);
    endInsertRows();
    return createIndex(row, 0, (void*)child);
}

void Fb2TreeView::insertNode()
{
    if (Fb2TreeModel * m = model()) {
        QModelIndex index = currentIndex();
        Fb2TreeItem * item = m->item(index);
        if (!item) return;

        Fb2TextElement element = item->element();
        while (!element.isNull()) {
            if (element.isSection() || element.isBody())
            {
                QUndoStack * undoStack = m_view.page()->undoStack();
                undoStack->beginMacro("Insert section");
                undoStack->push(new Fb2SectionCmd(*m_view.page(), element));
                undoStack->endMacro();

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

void Fb2TreeView::deleteNode()
{
    if (Fb2TreeModel * m = model()) {
        QModelIndex index = currentIndex();
        QModelIndex result = m->parent(index);
        setCurrentIndex(result);
        emit currentChanged(result, index);
        emit QTreeView::activated(result);
        m->removeRow(index.row(), result);
        scrollTo(result);
    }
}

Fb2TreeModel * Fb2TreeView::model()
{
    return qobject_cast<Fb2TreeModel*>(QTreeView::model());
}

void Fb2TreeView::moveCurrent(int dx, int dy)
{
    if (Fb2TreeModel * m = model()) {
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

void Fb2TreeView::moveUp()
{
    moveCurrent(0, -1);
}

void Fb2TreeView::moveDown()
{
    moveCurrent(0, +1);
}

void Fb2TreeView::moveLeft()
{
    moveCurrent(-1, 0);
}

void Fb2TreeView::moveRight()
{
    moveCurrent(+1, 0);
}

//---------------------------------------------------------------------------
//  Fb2TreeWidget
//---------------------------------------------------------------------------

Fb2TreeWidget::Fb2TreeWidget(Fb2TextEdit &view, QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout * layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setObjectName(QString::fromUtf8("verticalLayout"));

    m_tree = new Fb2TreeView(view, this);
    layout->addWidget(m_tree);

    m_tool = new QToolBar(this);
    layout->addWidget(m_tool);

    m_tree->initActions(m_tool);
}
