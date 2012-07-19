#include "fb2temp.hpp"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QImageReader>
#include <QUrl>
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
}

QNetworkReply * FbNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    if (request.url().scheme() == "fb2" && request.url().path() == m_view.url().path()) return imageRequest(op, request);
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

QNetworkReply * FbNetworkAccessManager::imageRequest(Operation op, const QNetworkRequest &request)
{
    QString name = request.url().fragment();
    QByteArray data = m_view.files().data(name);
    return new FbImageReply(op, request, data);
}
