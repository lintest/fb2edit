#include "fb2tree.hpp"

#include <QtDebug>
#include <QAction>
#include <QApplication>
#include <QVBoxLayout>
#include <QWebFrame>
#include <QWebPage>
#include <QTreeView>
#include <QUrl>

#include "fb2utils.h"
#include "fb2view.hpp"

//---------------------------------------------------------------------------
//  Fb2TreeItem
//---------------------------------------------------------------------------

Fb2TreeItem::Fb2TreeItem(QWebElement &element, Fb2TreeItem *parent, int number)
    : QObject(parent)
    , m_element(element)
    , m_parent(parent)
    , m_number(number)
{
    m_name = element.tagName().toLower();
    QString style = element.attribute("class").toLower();
    if (m_name == "div") {
        if (style == "title") {
            m_text = title(element);
            if (m_parent) m_parent->m_text += m_text += " ";
        } else if (style == "subtitle") {
            m_text = title(element);
        } else if (style == "body") {
            m_body = element.attribute("fb2_name");
        }
        if (!style.isEmpty()) m_name = style;
    } else if (m_name == "img") {
        m_name = "image";
        QUrl url = element.attribute("src");
        m_text = url.path();
    }
    addChildren(element);
}

Fb2TreeItem::~Fb2TreeItem()
{
    foreach (Fb2TreeItem * item, m_list) {
        delete item;
    }
}

QString Fb2TreeItem::title(const QWebElement &element)
{
    return element.toPlainText().left(255).simplified();
}

void Fb2TreeItem::addChildren(QWebElement &parent, bool direct, bool header)
{
    int number = 0;
    QWebElement child = parent.firstChild();
    while (!child.isNull()) {
        QString tag = child.tagName().toLower();
        if (tag == "div") {
            bool h = header;
            QString style = child.attribute("class").toLower();
            h = h || style == "description" || style == "stylesheet";
            if (!h || style == "annotation" || style == "history") {
                m_list << new Fb2TreeItem(child, this, direct ? number : -1);
            } else {
                addChildren(child, false, h);
            }
        } else if (header) {
            // skip
        } else if (tag == "img") {
            m_list << new Fb2TreeItem(child, this, direct ? number : -1);
        } else {
            addChildren(child, false, header);
        }
        child = child.nextSibling();
        number++;
    }
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

Fb2TreeItem * Fb2TreeItem::content(const Fb2TreeModel &model, int number, QModelIndex &index) const
{
    int row = 0;
    QList<Fb2TreeItem*>::const_iterator i;
    for (i = m_list.constBegin(); i != m_list.constEnd(); ++i) {
        if ((*i)->m_number == number) {
            index = model.index(row, 0, index);
            return *i;
        }
        row++;
    }
    return 0;
}

//---------------------------------------------------------------------------
//  Fb2TreeModel
//---------------------------------------------------------------------------

Fb2TreeModel::Fb2TreeModel(Fb2WebView &view, QObject *parent)
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
    QModelIndex index;
    Fb2TreeItem * parent = m_root;
    QStringList list = location.split(",");
    QStringListIterator iterator(list);
    while (parent && iterator.hasNext()) {
        QString str = iterator.next();
        if (str.left(5) == "HTML=") continue;
        int key = str.mid(str.indexOf("=")+1).toInt();
        Fb2TreeItem * child = parent->content(*this, key, index);
        parent = child;
    }
    return index;
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
            child->element().removeFromDocument();
            delete child;
        }
    }
    endRemoveRows();
    return true;
}

//---------------------------------------------------------------------------
//  Fb2TreeView
//---------------------------------------------------------------------------

Fb2TreeView::Fb2TreeView(Fb2WebView &view, QWidget *parent)
    : QTreeView(parent)
    , m_view(view)
{
    setHeaderHidden(true);
    setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(this, SIGNAL(activated(QModelIndex)), SLOT(activated(QModelIndex)));
    connect(m_view.page(), SIGNAL(loadFinished(bool)), SLOT(updateTree()));
    connect(m_view.page(), SIGNAL(contentsChanged()), SLOT(contentsChanged()));
    connect(m_view.page(), SIGNAL(selectionChanged()), SLOT(selectionChanged()));

    m_timerSelect.setInterval(1000);
    m_timerSelect.setSingleShot(true);
    connect(&m_timerSelect, SIGNAL(timeout()), SLOT(selectTree()));

    m_timerUpdate.setInterval(1000);
    m_timerUpdate.setSingleShot(true);
    connect(&m_timerUpdate, SIGNAL(timeout()), SLOT(updateTree()));

    QMetaObject::invokeMethod(this, "updateTree", Qt::QueuedConnection);
}

void Fb2TreeView::initToolbar(QToolBar *toolbar)
{
    QAction * act;

    act = new QAction(FB2::icon("list-add"), tr("&Insert"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("Insert"));
    act->setPriority(QAction::LowPriority);
    connect(act, SIGNAL(triggered()), SLOT(insertNode()));
    toolbar->addAction(act);
    addAction(act);

    act = new QAction(FB2::icon("list-remove"), tr("&Delete"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(QKeySequence("Delete"));
    act->setPriority(QAction::LowPriority);
    connect(act, SIGNAL(triggered()), SLOT(deleteNode()));
    toolbar->addAction(act);
    addAction(act);

    toolbar->addSeparator();

    act = new QAction(FB2::icon("go-up"), tr("&Up"), this);
    connect(act, SIGNAL(triggered()), SLOT(moveUp()));
    toolbar->addAction(act);
    addAction(act);

    act = new QAction(FB2::icon("go-down"), tr("&Down"), this);
    connect(act, SIGNAL(triggered()), SLOT(moveDown()));
    toolbar->addAction(act);
    addAction(act);

    act = new QAction(FB2::icon("go-previous"), tr("&Left"), this);
    connect(act, SIGNAL(triggered()), SLOT(moveLeft()));
    toolbar->addAction(act);
    addAction(act);

    act = new QAction(FB2::icon("go-next"), tr("&Right"), this);
    connect(act, SIGNAL(triggered()), SLOT(moveRight()));
    toolbar->addAction(act);
    addAction(act);
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
        QWebFrame * frame = m->view().page()->mainFrame();
        static const QString javascript = FB2::read(":/js/get_location.js");
        QString location = frame->evaluateJavaScript(javascript).toString();
        QModelIndex index = m->index(location);
        if (!index.isValid()) return;
        setCurrentIndex(index);
        scrollTo(index);
    }
}

void Fb2TreeView::updateTree()
{
    Fb2TreeModel * model = new Fb2TreeModel(m_view, this);
    setModel(model);
    selectTree();
}

QModelIndex Fb2TreeModel::append(const QModelIndex &parent)
{
    Fb2TreeItem * owner = item(parent);
    if (!owner || owner == m_root) return QModelIndex();
    int row = owner->count();
    owner->element().appendInside("<div class=section><div class=title><p><br/></p></div><p><br/></p></div>");
    QWebElement element = owner->element().lastChild();
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

        QString n = item->name();
        if (n != "section" && n != "body") {
            index = m->parent(index);
            item = item->parent();
            n = item->name();
            if (n != "section" && n != "body") return;
        }

        QModelIndex result = m->append(index);
        if (!result.isValid()) return;
        setCurrentIndex(result);
        emit QTreeView::currentChanged(result, index);
        emit QTreeView::activated(result);
        scrollTo(result);
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

Fb2TreeWidget::Fb2TreeWidget(Fb2WebView &view, QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout * layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setObjectName(QString::fromUtf8("verticalLayout"));

    m_tree = new Fb2TreeView(view, this);
    m_tree->setContextMenuPolicy(Qt::ActionsContextMenu);
    layout->addWidget(m_tree);

    m_tool = new QToolBar(this);
    layout->addWidget(m_tool);

    m_tree->initToolbar(m_tool);
}
