#include "fb2tree.hpp"

#include <QtDebug>
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
            QString name = element.attribute("name");
            if (!name.isEmpty()) style += " name=" + name;
        }
        if (!style.isEmpty()) m_name = style;
    } else if (m_name == "img") {
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

void Fb2TreeItem::addChildren(QWebElement &parent, bool direct)
{
    int number = 0;
    QWebElement child = parent.firstChild();
    while (!child.isNull()) {
        QString tag = child.tagName().toLower();
        if (tag == "div") {
            m_list << new Fb2TreeItem(child, this, direct ? number : -1);
        } else if (tag == "img") {
            m_list << new Fb2TreeItem(child, this, direct ? number : -1);
        } else {
            addChildren(child, false);
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
    return QString("<%1> %2").arg(m_name).arg(m_text);
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
    Fb2TreeItem *node = item(index);
    if (!node) return;
    QWebFrame *frame = m_view.page()->mainFrame();
    frame->scroll(0, node->pos().y() - frame->scrollPosition().y());
    QString selector = QString("var element=%1;").arg(node->selector());
    Fb2WebView::selectText(&m_view, selector);
    m_view.setFocus();
}

QModelIndex Fb2TreeModel::index(const QString &location, QModelIndex current) const
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

    while (current.isValid()) {
        if (current == index) return QModelIndex();
        current = this->parent(current);
    }

    return index;
}

//---------------------------------------------------------------------------
//  Fb2TreeView
//---------------------------------------------------------------------------

Fb2TreeView::Fb2TreeView(Fb2WebView &view, QWidget *parent)
    : QTreeView(parent)
    , m_view(view)
{
    setHeaderHidden(true);
    connect(&m_view, SIGNAL(loadFinished(bool)), SLOT(updateTree()));
    connect(this, SIGNAL(activated(QModelIndex)), SLOT(activated(QModelIndex)));
    connect(m_view.page(), SIGNAL(contentsChanged()), SLOT(contentsChanged()));
    connect(m_view.page(), SIGNAL(selectionChanged()), SLOT(selectionChanged()));

    m_timerSelect.setInterval(1000);
    m_timerSelect.setSingleShot(true);
    connect(&m_timerSelect, SIGNAL(timeout()), SLOT(selectTree()));

    m_timerUpdate.setInterval(1000);
    m_timerUpdate.setSingleShot(true);
    connect(&m_timerUpdate, SIGNAL(timeout()), SLOT(updateTree()));
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
    if (!model()) return;
    Fb2TreeModel *model = static_cast<Fb2TreeModel*>(this->model());

    disconnect(m_view.page(), SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
    model->selectText(index);
    connect(m_view.page(), SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
}

void Fb2TreeView::selectTree()
{
    if (model() == 0) return;
    Fb2TreeModel * model = static_cast<Fb2TreeModel*>(this->model());
    QWebFrame * frame = model->view().page()->mainFrame();
    static const QString javascript = FB2::read(":/js/get_location.js");
    QString location = frame->evaluateJavaScript(javascript).toString();
    QModelIndex index = model->index(location, currentIndex());
    if (!index.isValid()) return;

    disconnect(this, SIGNAL(activated(QModelIndex)), this, SLOT(activated(QModelIndex)));
    setCurrentIndex(index);
    scrollTo(index);
    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(activated(QModelIndex)));
}

void Fb2TreeView::updateTree()
{
    Fb2TreeModel * model = new Fb2TreeModel(m_view, this);
    setModel(model);
    selectTree();
}

//---------------------------------------------------------------------------
//  Fb2TreeWidget
//---------------------------------------------------------------------------

Fb2TreeWidget::Fb2TreeWidget(QWidget* parent = 0, Qt::WindowFlags f = 0)
    : QWidget(parent, f)
{
}
