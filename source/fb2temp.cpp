#include "fb2temp.hpp"

#include <QAbstractListModel>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QImageReader>
#include <QUrl>
#include <QVBoxLayout>
#include <QtDebug>

#include "fb2text.hpp"

//---------------------------------------------------------------------------
//  FbTemporaryFile
//---------------------------------------------------------------------------

FbTemporaryFile::FbTemporaryFile(const QString &name)
    : QTemporaryFile()
    , m_name(name)
{
}

qint64 FbTemporaryFile::write(const QByteArray &data)
{
    open();
    if (m_hash.isEmpty()) m_hash = md5(data);
    qint64 size = QTemporaryFile::write(data);
    close();

    return size;
}

QString FbTemporaryFile::md5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toBase64();
}

QByteArray FbTemporaryFile::data()
{
    open();
    QByteArray data = readAll();
    close();
    return data;
}

//---------------------------------------------------------------------------
//  FbTemporaryList
//---------------------------------------------------------------------------

FbTemporaryList::FbTemporaryList()
{
}

FbTemporaryList::~FbTemporaryList()
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) delete it.next();
}

QString FbTemporaryList::add(const QString &path, const QByteArray &data)
{
    QString hash = FbTemporaryFile::md5(data);
    QString name = this->name(hash);
    if (name.isEmpty()) {
        name = newName(path);
        FbTemporaryFile * temp = new FbTemporaryFile(name);
        temp->setHash(hash);
        temp->write(data);
        append(temp);
    }
    return name;
}

QString FbTemporaryList::newName(const QString &path)
{
    QFileInfo info(path);
    QString name = info.fileName();
    if (!exists(name)) return name;
    QString base = info.baseName();
    QString suff = info.suffix();
    for (int i = 1; ; i++) {
        QString name = QString("%1(%2).%3").arg(base).arg(i).arg(suff);
        if (exists(name)) continue;
        return name;
    }
}

FbTemporaryFile * FbTemporaryList::get(const QString &name) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbTemporaryFile * file = it.next();
        if (file->name() == name) return file;
    }
    return NULL;
}

QByteArray FbTemporaryList::data(const QString &name) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbTemporaryFile *file = it.next();
        if (file->name() == name) return file->data();
    }
    return QByteArray();
}

const QString & FbTemporaryList::set(const QString &name, const QByteArray &data, const QString &hash)
{
    FbTemporaryFile * file = get(name);
    if (!file) append(file = new FbTemporaryFile(name));
    file->setHash(hash);
    file->write(data);
    return file->hash();
}

QString FbTemporaryList::name(const QString &hash) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbTemporaryFile *file = it.next();
        if (file->hash() == hash) return file->name();
    }
    return QString();
}

bool FbTemporaryList::exists(const QString &name) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbTemporaryFile *file = it.next();
        if (file->name() == name) return true;
    }
    return false;
}

#if 0

//---------------------------------------------------------------------------
//  FbNetworkDiskCache
//---------------------------------------------------------------------------

QNetworkCacheMetaData FbNetworkDiskCache::metaData(const QUrl &url)
{
    qCritical() << url.toString();
    return QNetworkDiskCache::metaData(url);
}

QIODevice * FbNetworkDiskCache::data(const QUrl &url)
{
    qCritical() << url.toString();
    return QNetworkDiskCache::data(url);
}

#endif

//---------------------------------------------------------------------------
//  FbImageReply
//---------------------------------------------------------------------------

FbImageReply::FbImageReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request, const QByteArray &data)
    : QNetworkReply()
    , content(data)
    , offset(0)
{
    setOperation(op);
    setRequest(request);
    setUrl(request.url());
    open(ReadOnly | Unbuffered);
    setHeader(QNetworkRequest::ContentLengthHeader, QVariant(content.size()));
    setAttribute(QNetworkRequest::CacheSaveControlAttribute, QVariant(false));
    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}

qint64 FbImageReply::readData(char *data, qint64 maxSize)
{
    if (offset >= content.size()) return -1;
    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    qint64 number = qMin(maxSize, content.size() - offset);
    memcpy(data, content.constData() + offset, number);
    offset += number;
    return number;
}

//---------------------------------------------------------------------------
//  FbNetworkAccessManager
//
//    http://doc.trolltech.com/qq/32/qq32-webkit-protocols.html
//---------------------------------------------------------------------------

FbNetworkAccessManager::FbNetworkAccessManager(FbTextEdit &view)
    : QNetworkAccessManager(&view)
    , m_view(view)
{
    QWebSettings::clearMemoryCaches();
}

QNetworkReply * FbNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    if (request.url().scheme() == "fb2" && request.url().path() == m_view.url().path()) return imageRequest(op, request);
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

QNetworkReply * FbNetworkAccessManager::imageRequest(Operation op, const QNetworkRequest &request)
{
    QString name = request.url().fragment();
    QByteArray data = m_files.data(name);
    return new FbImageReply(op, request, data);
}

QString FbNetworkAccessManager::name(int index) const
{
    if (0 <= index && index < count()) {
        return m_files[index]->name();
    }
    return QString();
}

QByteArray FbNetworkAccessManager::data(int index) const
{
    if (0 <= index && index < count()) {
        return m_files[index]->data();
    }
    return QByteArray();
}

void FbNetworkAccessManager::data(QString name, QByteArray data)
{
    m_files.set(name, data);
}

//---------------------------------------------------------------------------
//  FbListModel
//---------------------------------------------------------------------------

FbListModel::FbListModel(FbNetworkAccessManager &files, QObject *parent)
    : QAbstractListModel(parent)
    , m_files(files)
{
}

int FbListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_files.count();
}

QVariant FbListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        switch (role) {
            case Qt::DisplayRole: return m_files.name(index.row());
        }
    }
    return QVariant();
}

//---------------------------------------------------------------------------
//  FbListView
//---------------------------------------------------------------------------

#include <QSplitter>
#include <QScrollArea>

FbListView::FbListView(FbNetworkAccessManager &files, QWidget *parent)
    : QListView(parent)
    , m_files(files)
{
    m_label = new QLabel(this);
    m_label->setScaledContents(true);
}

void FbListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QListView::currentChanged(current, previous);

    int row = current.row();
    if (0 <= row && row < m_files.count()) {
        QByteArray data = m_files.data(row);
        QPixmap pixmap;
        pixmap.loadFromData(data);
        m_label->setPixmap(pixmap);
        m_label->resize(pixmap.size());
    }
}

//---------------------------------------------------------------------------
//  FbListWidget
//---------------------------------------------------------------------------

FbListWidget::FbListWidget(FbTextEdit &view, QWidget* parent)
    : QWidget(parent)
    , m_view(view)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);

    m_list = new FbListView(view.files(), splitter);
    splitter->addWidget(m_list);

    QScrollArea * scroll = new QScrollArea(splitter);
    scroll->setWidget(m_list->label());
    splitter->addWidget(scroll);

    splitter->setSizes(QList<int>() << 1 << 1);
    layout->addWidget(splitter);

    connect(&m_view, SIGNAL(loadFinished(bool)), SLOT(loadFinished(bool)));
    loadFinished(true);

//    m_tool = new QToolBar(this);
//    layout->addWidget(m_tool);
}

void FbListWidget::loadFinished(bool ok)
{
    m_list->setModel(new FbListModel(m_view.files(), this));
    m_list->reset();
}
