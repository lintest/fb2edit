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

class FbTemporaryFile : public QTemporaryFile
{
    Q_OBJECT
public:
    static QString md5(const QByteArray &data);
public:
    explicit FbTemporaryFile(const QString &name);
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

class FbTemporaryList : public QList<FbTemporaryFile*>
{
public:
    explicit FbTemporaryList();
    virtual ~FbTemporaryList();

    QString add(const QString &path, QByteArray &data);
    bool exists(const QString &name) const;
    FbTemporaryFile * get(const QString &name) const;
    const QString & set(const QString &name, QByteArray data, const QString &hash = QString());
    QString name(const QString &hash) const;
    QByteArray data(const QString &name) const;
private:
    QString newName(const QString &path);
};

typedef QListIterator<FbTemporaryFile*> FbTemporaryIterator;

#if 0

class FbNetworkDiskCache : public QNetworkDiskCache
{
public:
    explicit FbNetworkDiskCache(QObject *parent = 0) : QNetworkDiskCache(parent) {}
    QNetworkCacheMetaData metaData(const QUrl &url);
    QIODevice *data(const QUrl &url);
};

#endif

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

class FbNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    explicit FbNetworkAccessManager(QObject *parent = 0);
    FbTemporaryList & files() { return m_files; }
    void setPath(const QString &path) { m_path = path; }

public slots:
    void data(const QString &name, const QByteArray &data);

public:
    QString add(const QString &path, QByteArray &data) { return m_files.add(path, data); }
    bool exists(const QString &name) const { return m_files.exists(name); }
    FbTemporaryFile * get(const QString &name) const { return m_files.get(name); }
    int count() const { return m_files.count(); }
    QByteArray data(int index) const;
    QVariant info(int row, int col) const;

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = 0);

private:
    QNetworkReply *imageRequest(Operation op, const QNetworkRequest &request);

private:
    FbTemporaryList m_files;
    QString m_path;
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
    explicit FbListView(FbNetworkAccessManager *files, QWidget *parent = 0);
    FbListModel *model() const;

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

signals:
    void showImage(const QString &name);

private:
    FbNetworkAccessManager &m_files;
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
