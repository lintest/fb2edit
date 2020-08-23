#include "fb2head.hpp"

#include <QtDebug>
#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QToolBar>
#include <QWebFrame>
#include <QWebPage>
#include <QWebView>
#include <QItemDelegate>
#include <QTreeView>

#include "fb2text.hpp"
#include "fb2utils.h"

//---------------------------------------------------------------------------
//  FbScheme::Fb
//---------------------------------------------------------------------------

FbScheme::FbDom::FbDom()
{
    QFile file(":/fb2/FictionBook2.1.xsd");
    if (file.open(QIODevice::ReadOnly)) setContent(&file);
}

//---------------------------------------------------------------------------
//  FbScheme
//---------------------------------------------------------------------------

const QDomDocument & FbScheme::fb2()
{
    static const FbDom doc;
    return doc;
}

FB2_BEGIN_KEYHASH(FbScheme)
    FB2_KEY( XsElement     , "xs:element"     );
    FB2_KEY( XsChoice      , "xs:choice"      );
    FB2_KEY( XsComplexType , "xs:complexType" );
    FB2_KEY( XsSequence    , "xs:sequence"    );
FB2_END_KEYHASH

void FbScheme::items(QStringList &list) const
{
    FbScheme child = typeScheme().firstChildElement();
    while (!child.isNull()) {
        switch (toKeyword(child.tagName())) {
            case XsElement: {
                    QString name = child.attribute("name");
                    if (!list.contains(name)) list << name;
                } break;
            case XsChoice:
            case XsComplexType:
            case XsSequence: {
                    child.items(list);
                } break;
            default: ;
        }
        child = child.nextSiblingElement();
    }
}

bool FbScheme::canEdit() const
{
    if (type() == "sequenceType") return true;
    FbScheme scheme = typeScheme();
    FbScheme child = scheme.firstChildElement();
    while (!child.isNull()) {
        switch (toKeyword(child.tagName())) {
            case XsElement:
                return false;
            case XsChoice:
            case XsComplexType:
            case XsSequence:
                if (!child.canEdit()) return false;
            default: ;
        }
        child = child.nextSiblingElement();
    }
    return true;
}

FbScheme FbScheme::typeScheme() const
{
    QString typeName = type();
    if (typeName.isEmpty()) return *this;

    FbScheme child = fb2().firstChildElement("xs:schema").firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == "xs:complexType") {
            if (child.attribute("name") == typeName) return child.element(typeName);
        }
        child = child.nextSiblingElement();
    }

    return FbScheme();
}

QString FbScheme::info() const
{
    QDomElement element = *this;
    if (element.isNull()) return QString();

    element = element.firstChildElement("xs:annotation");
    if (element.isNull()) return QString();

    element = element.firstChildElement("xs:documentation");
    if (element.isNull()) return QString();

    return element.text();
}

QString FbScheme::type() const
{
    if (isNull()) return QString();
    QString result = attribute("type");
    if (!result.isEmpty()) return result;
    FbScheme child = firstChildElement("xs:complexType").firstChildElement();
    QString tag;
    while (!child.isNull()) {
        tag = child.tagName();
        if (tag == "xs:complexContent" || tag == "xs:simpleContent") {
            return child.firstChildElement("xs:extension").attribute("base");
        }
        child = child.nextSiblingElement();
    }
    return QString();
}

FbScheme FbScheme::item(const QString &name) const
{
    FbScheme child = firstChildElement();
    while (!child.isNull()) {
        switch (toKeyword(child.tagName())) {
            case XsElement: {
                    if (child.attribute("name") == name) return child;
                } break;
            case XsChoice:
            case XsComplexType:
            case XsSequence: {
                    FbScheme result = child.item(name);
                    if (!result.isNull()) return result;
                } break;
            default: ;
        }
        child = child.nextSiblingElement();
    }
    return FbScheme();
}

FbScheme FbScheme::element(const QString &name) const
{
    FbScheme parent = *this;
    if (parent.isNull()) {
        parent = fb2().documentElement();
        parent = parent.element("FictionBook");
    }

    FbScheme child = parent.item(name);
    if (!child.isNull()) return child;

    QString type = this->type();
    if (type.isEmpty()) return *this;

    child = fb2().firstChildElement("xs:schema").firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == "xs:complexType") {
            if (child.attribute("name") == type) return child.element(name);
        }
        child = child.nextSiblingElement();
    }

    return FbScheme();
}

//---------------------------------------------------------------------------
//  FbHeadItem
//---------------------------------------------------------------------------

FbHeadItem::HintHash::HintHash()
{
    insert( "title-info"    , tr( "Book"        ));
    insert( "document-info" , tr( "File"        ));
    insert( "publish-info"  , tr( "Publish"     ));
    insert( "custom-info"   , tr( "Add-ons"     ));
    insert( "genre"         , tr( "Genre"       ));
    insert( "author"        , tr( "Author"      ));
    insert( "book-title"    , tr( "Title"       ));
    insert( "annotation"    , tr( "Annotation"  ));
    insert( "coverpage"     , tr( "Cover"       ));
    insert( "program-used"  , tr( "Editor"      ));
    insert( "date"          , tr( "Date"        ));
    insert( "lang"          , tr( "Language"    ));
    insert( "translator"    , tr( "Translator"  ));
    insert( "sequence"      , tr( "Sequence"    ));
    insert( "first-name"    , tr( "First name"  ));
    insert( "middle-name"   , tr( "Middle name" ));
    insert( "last-name"     , tr( "Last name"   ));
    insert( "history"       , tr( "History"     ));
}

FB2_BEGIN_KEYHASH(FbHeadItem)
    FB2_KEY( Genr   , "genre"     );
    FB2_KEY( Auth   , "author"    );
    FB2_KEY( Date   , "date"      );
    FB2_KEY( Cover  , "coverpage" );
    FB2_KEY( Image  , "image"     );
    FB2_KEY( Seqn   , "sequence"  );
FB2_END_KEYHASH

FbHeadItem::FbHeadItem(QWebElement &element, FbHeadItem *parent)
    : QObject(parent)
    , m_element(element)
    , m_parent(parent)
{
    m_name = element.tagName().toLower();
    if (m_name.left(3) == "fb:") {
        m_name = m_name.mid(3);
        if (m_name == "annotation") return;
        if (m_name == "history") return;
    } else if (m_name == "img") {
        m_name = "image";
    }
    addChildren(element);
}

FbHeadItem::~FbHeadItem()
{
    for (FbHeadItem * item: m_list) {
        delete item;
    }
}

FbHeadItem * FbHeadItem::append(const QString name)
{
    m_element.appendInside(QString("<fb:%1></fb:%1>").arg(name));
    QWebElement element = m_element.lastChild();
    if (name == "annotation" || name == "history") {
        element.appendInside("<p><br></p>");
    }
    FbHeadItem * child = new FbHeadItem(element, this);
    m_list << child;
    return child;
}

void FbHeadItem::addChildren(QWebElement &parent)
{
    QWebElement child = parent.firstChild();
    while (!child.isNull()) {
        QString tag = child.tagName().toLower();
        if (tag.left(3) == "fb:") {
            m_list << new FbHeadItem(child, this);
        } else if (tag == "img") {
            m_list << new FbHeadItem(child, this);
        } else {
            addChildren(child);
        }
        child = child.nextSibling();
    }
}

FbHeadItem * FbHeadItem::item(const QModelIndex &index) const
{
    int row = index.row();
    if (row < 0 || row >= m_list.size()) return NULL;
    return m_list[row];
}

FbHeadItem * FbHeadItem::item(int row) const
{
    if (row < 0 || row >= m_list.size()) return NULL;
    return m_list[row];
}

QString FbHeadItem::data(int col) const
{
    switch (col) {
        case 0: return QString("<%1> %2").arg(m_name).arg(hint());
        case 1: return value();
        case 2: return extra();
        case 3: return scheme().info();
        case 4: return scheme().type();
        case 5: return scheme().canEdit() ? "Yes" : "No";
        case 6: return scheme().attribute("minOccurs");
        case 7: return scheme().attribute("maxOccurs");
    }
    return QString();
}

void FbHeadItem::setData(const QString &text, int col)
{
    switch (col) {
        case 1: return setValue(text);
        case 2: return setExtra(text);
    }
}

QString FbHeadItem::hint() const
{
    static HintHash hints;
    HintHash::const_iterator it = hints.find(m_name);
    if (it == hints.end()) return QString();
    return it.value();
}

QString FbHeadItem::value() const
{
    switch (toKeyword(m_name)) {
        case Auth: {
            QString result = sub("last-name");
            result += " " + sub("first-name");
            result += " " + sub("middle-name");
            return result.simplified();
        } break;
        case Cover: {
            QString text;
            for (FbHeadItem * item: m_list) {
                if (item->m_name == "image") {
                    if (!text.isEmpty()) text += ", ";
                    text += item->value();
                }
            }
            return text;
        } break;
        case Image: {
            return m_element.attribute("src");
        } break;
        case Seqn: {
            return m_element.attribute("name");
        } break;
        default: ;
    }
    if (m_list.count()) return QString();
    return m_element.toPlainText().simplified();
}

QString FbHeadItem::extra() const
{
    switch (toKeyword(m_name)) {
        case Genr: return m_element.attribute("match");
        case Seqn: return m_element.attribute("number");
        case Date: return m_element.attribute("value");
        default: return QString();
    }
}

void FbHeadItem::setValue(const QString &text)
{
    switch (toKeyword(m_name)) {
        case Seqn: m_element.setAttribute("name", text); break;
        default: m_element.setPlainText(text);
    }
}

void FbHeadItem::setExtra(const QString &text)
{
    switch (toKeyword(m_name)) {
        case Date: m_element.setAttribute("value", text); break;
        case Genr: m_element.setAttribute("match", text); break;
        case Seqn: m_element.setAttribute("number", text); break;
        default: ;
    }
}

bool FbHeadItem::canEditExtra() const
{
    switch (toKeyword(m_name)) {
        case Date:
        case Genr:
        case Seqn:
            return true;
        default:
            return false;
    }
}

QString FbHeadItem::sub(const QString &key) const
{
    for (FbHeadItem * item: m_list) {
        if (item->m_name == key) return item->value();
    }
    return QString();
}

FbScheme FbHeadItem::scheme() const
{
    FbScheme parent = m_parent ? m_parent->scheme() : FbScheme();
    return parent.element(m_name);
}

void FbHeadItem::remove(int row)
{
    if (row < 0 || row >= count()) return;
    m_list[row]->m_element.removeFromDocument();
    m_list.removeAt(row);
}

//---------------------------------------------------------------------------
//  FbHeadModel
//---------------------------------------------------------------------------

FbHeadModel::FbHeadModel(QWebView &view, QObject *parent)
    : QAbstractItemModel(parent)
    , m_view(view)
    , m_root(NULL)
{
    QWebElement doc = view.page()->mainFrame()->documentElement();
    QWebElement head = doc.findFirst("fb\\:description");
    if (head.isNull()) return;
    m_root = new FbHeadItem(head);
}

FbHeadModel::~FbHeadModel()
{
    if (m_root) delete m_root;
}

void FbHeadModel::expand(QTreeView *view)
{
    QModelIndex parent = QModelIndex();
    int count = rowCount(parent);
    for (int i = 0; i < count; ++i) {
        QModelIndex child = index(i, 0, parent);
        FbHeadItem *node = item(child);
        if (!node) continue;
        view->expand(child);
        int count = rowCount(child);
        for (int j = 0; j < count; ++j) {
            QModelIndex ch = index(j, 0, child);
            FbHeadItem *node = item(ch);
            if (node) view->expand(ch);
        }
    }
}

FbHeadItem * FbHeadModel::item(const QModelIndex &index) const
{
    if (index.isValid()) {
        return static_cast<FbHeadItem*>(index.internalPointer());
    } else {
        return 0;
    }
}

int FbHeadModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
#ifdef QT_DEBUG
    return 8;
#else
    return 3;
#endif
}

QModelIndex FbHeadModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!m_root || row < 0 || column < 0) return QModelIndex();

    if (!parent.isValid() && m_root) {
        return createIndex(row, column, (void*)m_root);
    }

    if (FbHeadItem *owner = item(parent)) {
        if (FbHeadItem *child = owner->item(row)) {
            return createIndex(row, column, (void*)child);
        }
    }

    return QModelIndex();
}

QModelIndex FbHeadModel::parent(const QModelIndex &child) const
{
    if (FbHeadItem * node = static_cast<FbHeadItem*>(child.internalPointer())) {
        if (FbHeadItem * parent = node->parent()) {
            if (FbHeadItem * owner = parent->parent()) {
                return createIndex(owner->index(parent), 0, (void*)parent);
            } else {
                return createIndex(0, 0, (void*)parent);
            }
        }
    }
    return QModelIndex();
}

int FbHeadModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) return 0;
    if (!parent.isValid()) return m_root ? 1 : 0;
    FbHeadItem *owner = item(parent);
    return owner ? owner->count() : 0;
}

QVariant FbHeadModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();
    FbHeadItem * i = item(index);
    return i ? i->data(index.column()) : QVariant();
}

QVariant FbHeadModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Key");
            case 1: return tr("Value");
            case 2: return tr("Extra");
        #ifdef QT_DEBUG
            case 3: return tr("Info");
            case 4: return tr("Type");
            case 5: return tr("Can edit");
            case 6: return tr("Min");
            case 7: return tr("Max");
        #endif
        }
    }
    return QVariant();
}

bool FbHeadModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) return false;
    if (FbHeadItem * i = item(index)) {
        i->setData(value.toString(), index.column());
        emit dataChanged(index, index);
        return true;
    }
    return true;
}

Qt::ItemFlags FbHeadModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return 0;
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (canEdit(index)) flags |= Qt::ItemIsEditable;
    return flags;
}

bool FbHeadModel::canEdit(const QModelIndex &index) const
{
    switch (index.column()) {
        case 1: if (FbHeadItem * i = item(index)) return i->canEdit(); break;
        case 2: if (FbHeadItem * i = item(index)) return i->canEditExtra(); break;
    }
    return false;
}

//---------------------------------------------------------------------------
//  FbTreeView
//---------------------------------------------------------------------------

FbHeadEdit::FbHeadEdit(QWidget *parent, FbTextEdit *text)
    : QTreeView(parent)
    , m_text(text)
{
    QAction * act;

    setContextMenuPolicy(Qt::ActionsContextMenu);
    setStyleSheet("show-decoration-selected: 1;");

    actionInsert = act = new QAction(FbIcon("list-add"), tr("&Append"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(Qt::Key_Insert);
    act->setPriority(QAction::LowPriority);
    connect(act, SIGNAL(triggered()), SLOT(appendNode()));
    addAction(act);

    actionModify = act = new QAction(FbIcon("list-add"), tr("&Modify"), this);
    act->setPriority(QAction::LowPriority);

    actionDelete = act = new QAction(FbIcon("list-remove"), tr("&Delete"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setShortcut(Qt::Key_Delete);
    act->setPriority(QAction::LowPriority);
    connect(act, SIGNAL(triggered()), SLOT(removeNode()));
    addAction(act);

    //setItemDelegate(new QItemDelegate(this));
    setRootIsDecorated(false);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(this, SIGNAL(activated(QModelIndex)), SLOT(activated(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)), SLOT(collapsed(QModelIndex)));

    header()->setDefaultSectionSize(200);
}

void FbHeadEdit::setAction(Fb::Actions index, QAction *action)
{
    m_actions[index] = action;
}

void FbHeadEdit::connectActions(QToolBar *tool)
{
    tool->clear();
    tool->addSeparator();
    tool->addAction(actionInsert);
    tool->addAction(actionDelete);
}

void FbHeadEdit::disconnectActions()
{
    m_actions.disconnect();
}

FbHeadModel * FbHeadEdit::model() const
{
    return qobject_cast<FbHeadModel*>(QTreeView::model());
}

void FbHeadEdit::updateTree()
{
    FbHeadModel * model = new FbHeadModel(*m_text, this);
    setModel(model);
    model->expand(this);
}

void FbHeadEdit::editCurrent(const QModelIndex &index)
{
    FbHeadModel * m = qobject_cast<FbHeadModel*>(model());
    if (!m) return;

    FbHeadItem * item = m->item(index);
    if (!item) return;

    FbNodeEditDlg dlg(this, item->scheme(), item->element());
    if (dlg.exec()) {
        //TODO
    }
}

void FbHeadEdit::activated(const QModelIndex &index)
{
    if (model()->canEdit(index)) edit(index);
    showStatus(index);
}

void FbHeadEdit::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTreeView::currentChanged(current, previous);
    showStatus(current);
}

void FbHeadEdit::showStatus(const QModelIndex &current)
{
    if (!model()) return;
    if (!current.isValid()) return;
    QModelIndex parent = model()->parent(current);
    QModelIndex index = model()->index(current.row(), 3, parent);
    emit status(model()->data(index).toString());
}

void FbHeadEdit::collapsed(const QModelIndex &index)
{
    if (model() && !model()->parent(index).isValid()) {
        expand(index);
    }
}

void FbHeadEdit::appendNode()
{
    FbHeadModel * m = qobject_cast<FbHeadModel*>(model());
    if (!m) return;

    QModelIndex current = currentIndex();
    FbHeadItem * item = m->item(current);
    if (!item) return;

    QString name = item->name().toLower();
    if (name == "annotation" || name == "history") {
        current = m->parent(current);
        item = m->item(current);
    }

    QStringList list;
    item->scheme().items(list);
    if (list.count() == 0) {
        current = m->parent(current);
        item = m->item(current);
        if (!item) return;
        item->scheme().items(list);
    }

    FbNodeDlg dlg(this, item->scheme(), list);
    if (dlg.exec()) {
        QModelIndex child = m->append(current, dlg.value());
        if (child.isValid()) {
            expand(current);
            setCurrentIndex(child);
            scrollTo(child);
        }
    }
}

void FbHeadEdit::removeNode()
{
    FbHeadModel * m = qobject_cast<FbHeadModel*>(model());
    if (m) m->remove(currentIndex());
}

QModelIndex FbHeadModel::append(const QModelIndex &parent, const QString &name)
{
    FbHeadItem * owner = item(parent);
    if (!owner) return QModelIndex();
    int row = owner->count();
    beginInsertRows(parent, row, row);
    FbHeadItem * item = owner->append(name);
    endInsertRows();
    return createIndex(row, 0, (void*)item);
}

void FbHeadModel::remove(const QModelIndex &index)
{
    int r = index.row();
    QModelIndex p = parent(index);
    beginRemoveRows(p, r, r + 1);
    FbHeadItem * i = item(p);
    if (i) i->remove(r);
    endRemoveRows();
}

//---------------------------------------------------------------------------
//  FbNodeDlg
//---------------------------------------------------------------------------

FbNodeDlg::FbNodeDlg(QWidget *parent, const FbScheme &scheme, const QStringList &list)
    : QDialog(parent)
    , m_scheme(scheme)
{
    setWindowTitle(tr("Insert tag"));

    QGridLayout * layout = new QGridLayout(this);

    QLabel * label = new QLabel(this);
    label->setText(tr("Tag name:"));
    layout->addWidget(label, 0, 0, 1, 1);

    m_combo = new QComboBox(this);
    m_combo->setEditable(true);
    m_combo->addItems(list);
    layout->addWidget(m_combo, 0, 1, 1, 1);

    m_text = new QLabel(this);
    m_text->setStyleSheet(QString::fromUtf8("border-style:outset;border-width:1px;border-radius:10px;padding:6px;"));
    m_text->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
    m_text->setMinimumSize(QSize(300, 100));
    m_text->setWordWrap(true);
    layout->addWidget(m_text, 1, 0, 1, 2);

    QDialogButtonBox * buttons = new QDialogButtonBox(this);
    buttons->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttons->setOrientation(Qt::Horizontal);
    layout->addWidget(buttons, 2, 0, 1, 2);

    layout->setColumnStretch(1, 1);

    connect(m_combo, SIGNAL(editTextChanged(QString)), SLOT(comboChanged(QString)));
    connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), SLOT(reject()));

    if (list.count()) {
        m_combo->setCurrentIndex(0);
        comboChanged(list.first());
    }
}

void FbNodeDlg::comboChanged(const QString &text)
{
    m_text->setText(m_scheme.element(text).info());
}

QString FbNodeDlg::value() const
{
    return m_combo->currentText();
}

//---------------------------------------------------------------------------
//  FbNodeEditDlg
//---------------------------------------------------------------------------

FbNodeEditDlg::FbNodeEditDlg(QWidget *parent, const FbScheme &scheme, const QWebElement &element)
    : QDialog(parent)
    , m_scheme(scheme)
    , m_element(element)
{
    setWindowTitle(tr("Modify tag"));

    QGridLayout * layout = new QGridLayout(this);

    QLabel *label = new QLabel(this);
    label->setText(tr("Value:"));
    layout->addWidget(label, 0, 0, 1, 1);

    QLineEdit *edit = new QLineEdit(this);
    layout->addWidget(edit, 0, 1, 1, 1);

    QDialogButtonBox * buttons = new QDialogButtonBox(this);
    buttons->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttons->setOrientation(Qt::Horizontal);
    layout->addWidget(buttons, 2, 0, 1, 2);

    layout->setColumnStretch(1, 1);

    connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), SLOT(reject()));
}

//---------------------------------------------------------------------------
//  FbAuthorDlg
//---------------------------------------------------------------------------

FbAuthorDlg::FbAuthorDlg(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Author"));

    QFormLayout * layout = new QFormLayout(this);

    add(layout, "last-name", tr("Last name"));
    add(layout, "first-name", tr("First name"));
    add(layout, "moddle-name", tr("Middle name"));
    add(layout, "nickname", tr("Nic name"));
    add(layout, "home-page", tr("Home page"));
    add(layout, "email", tr("E-mail"));

    QDialogButtonBox * buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    layout->addRow(buttonBox);

    QObject::connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

void FbAuthorDlg::add(QFormLayout *layout, const QString &key, const QString &text)
{
    QLabel * label = new QLabel(this);
    label->setText(text);

    QLineEdit * field = new QLineEdit(this);
    field->setMinimumWidth(200);
    m_fields[key] = field;

    layout->addRow(label, field);
}
