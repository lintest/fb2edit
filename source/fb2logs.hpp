#ifndef FB2LOGS_H
#define FB2LOGS_H

#include <QAbstractListModel>
#include <QListView>
#include <QDockWidget>

class FbLogItem
{
public:
    enum Level {
        Message,
        Warring,
        Error,
        Fatal
    };

    FbLogItem(Level level, int row, int col, const QString &msg);
    FbLogItem(Level level, const QString &msg);

    Level level() const {return m_level; }
    const QString & msg() const { return m_msg; }
    int row() const {return m_row; }
    int col() const {return m_row; }

private:
    Level m_level;
    QString m_msg;
    int m_row;
    int m_col;

};

class FbLogModel : public QAbstractListModel
{
    Q_OBJECT

public:
    FbLogModel(QObject *parent = 0);

public:
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

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

private:
    QList<FbLogItem*> items;
};

class FbLogDock: public QDockWidget
{
    Q_OBJECT

public:
    explicit FbLogDock(const QString &title, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    void append(const QString &message);

private:
    FbLogList *m_list;
};

#endif // FB2LOGS_H
