#ifndef FB2TEMP_H
#define FB2TEMP_H

#include <QByteArray>
#include <QLabel>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QTemporaryFile>
#include <QTreeView>
#include <QWebView>

class FbTextEdit;

class FbNetworkAccessManager;

class FbBinary : public QTemporaryFile
{
    Q_OBJECT
public:
    static QString md5(const QByteArray &data);
public:
    explicit FbBinary(const QString &name);
    inline qint64 write(QByteArray &data);
    void setHash(const QString &hash) { m_hash = hash; }
    const QString & hash() const { return m_hash; }
    const QString & name() const { return m_name; }
    const QString & type() const { return m_type; }
    qint64 size() const { return m_size; }
    QByteArray data();
private:
    const QString m_name;
    QString m_hash;
    QString m_type;
    qint64 m_size;
};

typedef QList<FbBinary*> FbBinatyList;

class FbStore : public QObject, private FbBinatyList
{
    Q_OBJECT
public:
    explicit FbStore(QObject *parent);
    virtual ~FbStore();
    QString add(const QString &path, QByteArray &data);
    bool exists(const QString &name) const;
    FbBinary * get(const QString &name) const;
    const QString & set(const QString &name, QByteArray data, const QString &hash = QString());
    QString name(const QString &hash) const;
    QByteArray data(const QString &name) const;
public slots:
    void binary(const QString &name, const QByteArray &data);
public:
    inline FbBinary * at(int i) const { return FbBinatyList::at(i); }
    inline int count() const { return FbBinatyList::count(); }
private:
    QString newName(const QString &path);
};

typedef QListIterator<FbBinary*> FbTemporaryIterator;

#if 0

class FbNetworkDiskCache : public QNetworkDiskCache
{
public:
    explicit FbNetworkDiskCache(QObject *parent = 0) : QNetworkDiskCache(parent) {}
    QNetworkCacheMetaData metaData(const QUrl &url);
    QIODevice *data(const QUrl &url);
};

#endif

class FbNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    explicit FbNetworkAccessManager(QObject *parent = 0);
    void setStore(const QUrl url, FbStore *store);
    FbStore *store() const { return m_store; }

public:
    QString add(const QString &path, QByteArray &data) { return m_store->add(path, data); }
    bool exists(const QString &name) const { return m_store->exists(name); }
    FbBinary * get(const QString &name) const { return m_store->get(name); }
    int count() const { return m_store->count(); }
    QByteArray data(int index) const;
    QVariant info(int row, int col) const;

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = 0);

private:
    FbStore *m_store;
    QString m_path;
};

class FbImageReply : public QNetworkReply
{
    Q_OBJECT
public:
    explicit FbImageReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request, const QByteArray &data);
    qint64 bytesAvailable() const { return content.size(); }
    bool isSequential() const { return true; }
    void abort() { close(); }

protected:
    qint64 readData(char *data, qint64 maxSize);

private:
    QByteArray content;
    qint64 offset;
};

class FbListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit FbListModel(FbTextEdit *text, QObject *parent = 0);

public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    FbNetworkAccessManager *files() const;

private:
    FbTextEdit *m_text;
};

class FbListView : public QTreeView
{
    Q_OBJECT

public:
    explicit FbListView(QWidget *parent = 0);
    FbListModel *model() const;

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

signals:
    void showImage(const QString &name);
};

class FbListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FbListWidget(FbTextEdit *text, QWidget* parent = 0);

    QSize sizeHint() const { return QSize(200,200); }

public slots:
    void showImage(const QString &name);

private slots:
    void loadFinished();

private:
    FbTextEdit *m_text;
    FbListView *m_list;
    QWebView *m_view;
};

#endif // FB2TEMP_H
