#ifndef FB2LOGS_H
#define FB2LOGS_H

#include <QAbstractListModel>
#include <QListView>
#include <QDockWidget>

class FbLogModel : public QAbstractListModel
{
    Q_OBJECT

public:
    FbLogModel(QObject *parent = 0);
    void add(QtMsgType type, int row, int col, const QString &msg);
    void add(QtMsgType type, const QString &msg);

public:
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

signals:
    void changeCurrent(const QModelIndex &index);

private:
    class FbLogItem
    {
    public:
        FbLogItem(QtMsgType type, int row, int col, const QString &msg)
            : m_type(type), m_msg(msg), m_row(row), m_col(col) {}

        FbLogItem(QtMsgType type, const QString &msg)
            : m_type(type), m_msg(msg), m_row(0), m_col(0) {}

        const QString & msg() const { return m_msg; }
        QtMsgType type() const { return m_type; }
        int row() const { return m_row; }
        int col() const { return m_row; }
        QVariant icon() const;

    private:
        QtMsgType m_type;
        QString m_msg;
        int m_row;
        int m_col;

    };

private:
    QList<FbLogItem*> m_list;
};

class FbLogList: public QListView
{
    Q_OBJECT

public:
    explicit FbLogList(QWidget *parent = 0);

    QSize sizeHint() const {
        QSize sh = QListView::sizeHint();
        sh.setHeight(40);
        return sh;
    }
};

class FbLogDock: public QDockWidget
{
    Q_OBJECT

public:
    explicit FbLogDock(const QString &title, QWidget *parent = 0, Qt::WindowFlags flags = {});
    void append(QtMsgType type, const QString &message);

private:
    FbLogModel *m_model;
    FbLogList *m_list;
};

#endif // FB2LOGS_H
