#ifndef FB2HEAD_H
#define FB2HEAD_H

#include <QAbstractItemModel>
#include <QDialog>
#include <QDomDocument>
#include <QDomElement>
#include <QMap>
#include <QTreeView>
#include <QWebElement>
#include <QWebView>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QFormLayout;
class QLabel;
class QToolBar;
class QLineEdit;
QT_END_NAMESPACE

#include "fb2mode.h"
#include "fb2xml.h"

class FbTextEdit;

class FbScheme : public QDomElement
{
    FB2_BEGIN_KEYLIST
        XsElement,
        XsChoice,
        XsComplexType,
        XsSequence,
    FB2_END_KEYLIST

private:
    class FbDom : public QDomDocument { public: FbDom(); };

public:
    FbScheme() {}
    FbScheme(const FbScheme &x) : QDomElement(x) {}
    FbScheme(const QDomElement &x) : QDomElement(x) {}
    FbScheme& operator=(const FbScheme &x) { QDomElement::operator=(x); return *this; }

    void setAction(Fb::Actions index, QAction *action);
    void connectActions();
    void disconnectActions();

    static const QDomDocument & fb2();
    FbScheme element(const QString &name) const;
    void items(QStringList &list) const;
    bool canEdit() const;
    QString info() const;
    QString type() const;

private:
    FbScheme typeScheme() const;
    FbScheme item(const QString &name) const;
    FbActionMap m_actions;
};

class FbHeadItem: public QObject
{
    Q_OBJECT

    FB2_BEGIN_KEYLIST
        Genr,
        Auth,
        Date,
        Cover,
        Image,
        Seqn,
    FB2_END_KEYLIST

public:
    explicit FbHeadItem(QWebElement &element, FbHeadItem *parent = 0);

    virtual ~FbHeadItem();

    FbHeadItem * append(const QString name);

    void remove(int row);

    FbHeadItem * item(const QModelIndex &index) const;

    FbHeadItem * item(int row) const;

    int index(FbHeadItem * child) const {
        return m_list.indexOf(child);
    }

    int count() const {
        return m_list.size();
    }

    FbHeadItem * parent() const {
        return m_parent;
    }

    QWebElement element() const {
        return m_element;
    }

    QString data(int col = 0) const;

    void setData(const QString &text, int col = 0);

    const QString & name() const {
        return m_name;
    }

    QString sub(const QString &key) const;

    FbScheme scheme() const;

    bool canEdit() const {
        return scheme().canEdit();
    }

    bool canEditExtra() const;

private:
    class HintHash : public QHash<QString, QString>
    {
    public:
        explicit HintHash();
    };

private:
    void addChildren(QWebElement &parent);
    void setValue(const QString &text);
    void setExtra(const QString &text);
    QString value() const;
    QString extra() const;
    QString hint() const;

private:
    QList<FbHeadItem*> m_list;
    QWebElement m_element;
    FbHeadItem * m_parent;
    QString m_name;
};

class FbHeadModel: public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit FbHeadModel(QWebView &view, QObject *parent = 0);
    virtual ~FbHeadModel();
    void expand(QTreeView *view);
    FbHeadItem * item(const QModelIndex &index) const;
    QModelIndex append(const QModelIndex &parent, const QString &name);
    bool canEdit(const QModelIndex &index) const;
    void remove(const QModelIndex &index);

public:
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

private:
    QWebView & m_view;
    FbHeadItem * m_root;
};

class FbHeadEdit : public QTreeView
{
    Q_OBJECT

public:
    explicit FbHeadEdit(QWidget *parent, FbTextEdit *text);
    void initToolbar(QToolBar &toolbar);
    FbHeadModel * model() const;

    void setAction(Fb::Actions index, QAction *action);
    void connectActions(QToolBar *tool);
    void disconnectActions();

signals:
    void status(const QString &text);

public slots:
    void editCurrent(const QModelIndex &index);
    void updateTree();

private slots:
    void activated(const QModelIndex &index);
    void collapsed(const QModelIndex &index);
    void appendNode();
    void removeNode();

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    void showStatus(const QModelIndex &current);

private:
    FbTextEdit * m_text;
    QAction * actionInsert;
    QAction * actionModify;
    QAction * actionDelete;
    FbActionMap m_actions;
};

class FbNodeDlg : public QDialog
{
    Q_OBJECT

public:
    explicit FbNodeDlg(QWidget *parent, const FbScheme &scheme, const QStringList &list);
    QString value() const;

private slots:
    void comboChanged(const QString &text);

private:
    const FbScheme &m_scheme;
    QComboBox * m_combo;
    QLabel * m_text;
};

class FbNodeEditDlg : public QDialog
{
    Q_OBJECT

public:
    explicit FbNodeEditDlg(QWidget *parent, const FbScheme &scheme, const QWebElement &element);

private:
    const FbScheme &m_scheme;
    QWebElement m_element;
};

class FbAuthorDlg : public QDialog
{
    Q_OBJECT

public:
    explicit FbAuthorDlg(QWidget *parent);

private:
    void add(QFormLayout *layout, const QString &key, const QString &text);

private:
    QMap<QString, QLineEdit*> m_fields;
};

#endif // FB2HEAD_H
