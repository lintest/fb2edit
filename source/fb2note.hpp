#ifndef FB2NOTE_H
#define FB2NOTE_H

#include <QAbstractListModel>
#include <QDialog>
#include <QList>
#include <QTreeView>
#include <QWebElement>
#include <QWebElementCollection>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QToolBar;
class QWebView;
QT_END_NAMESPACE

class FbTextPage;
class FbTextBase;
class FbTextEdit;

class FbNoteDlg : public QDialog
{
    Q_OBJECT

public:
    explicit FbNoteDlg(FbTextBase *text);

private slots:
    void loadFinished();

private:
    QComboBox *m_key;
    FbTextBase *m_text;
    QLineEdit *m_title;
    QToolBar *m_toolbar;
};

class FbNotesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit FbNotesModel(FbTextPage *page, QObject *parent = 0);

public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QWebElementCollection collection;
    FbTextPage *m_page;
};

class FbNotesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FbNotesWidget(FbTextEdit *text, QWidget* parent = 0);
    QSize sizeHint() const { return QSize(200,200); }

public slots:
    void showCurrent(const QString &name);

private slots:
    void loadFinished();

private:
    FbTextEdit *m_text;
    QTreeView *m_list;
    QWebView *m_view;
};

#endif // FB2NOTE_H
