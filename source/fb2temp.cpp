#include "fb2temp.hpp"

#include <QCryptographicHash>
#include <QUrl>
#include <QtDebug>

#include "fb2view.hpp"

//---------------------------------------------------------------------------
//  Fb2TemporaryFile
//---------------------------------------------------------------------------

Fb2TemporaryFile::Fb2TemporaryFile(const QString &name, const QString &hash)
    : QTemporaryFile()
    , m_name(name)
    , m_hash(hash)
{
    open();
}

qint64 Fb2TemporaryFile::write(const QByteArray &data)
{
    if (m_hash.isEmpty()) m_hash = md5(data);
    qint64 size = QTemporaryFile::write(data);
    close();
    open();
    return size;
}

QString Fb2TemporaryFile::md5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toBase64();
}

//---------------------------------------------------------------------------
//  Fb2TemporaryList
//---------------------------------------------------------------------------

Fb2TemporaryList::Fb2TemporaryList()
{
}

Fb2TemporaryList::~Fb2TemporaryList()
{
    Fb2TemporaryIterator it(*this);
    while (it.hasNext()) delete it.next();
}

Fb2TemporaryFile & Fb2TemporaryList::get(const QString &name, const QString &hash)
{
    Fb2TemporaryIterator it(*this);
    while (it.hasNext()) {
        Fb2TemporaryFile *file = it.next();
        if (file->name() == name) return *file;
    }
    Fb2TemporaryFile * file = new Fb2TemporaryFile(name, hash);
    append(file);
    return *file;
}

QByteArray Fb2TemporaryList::fileData(const QString &name) const
{
    Fb2TemporaryIterator it(*this);
    while (it.hasNext()) {
        Fb2TemporaryFile *file = it.next();
        if (file->name() == name) {
            QByteArray data = file->readAll();
            file->close();
            file->open();
            return data;
        }
    }
    return QByteArray();
}

QString Fb2TemporaryList::set(const QString &name, const QByteArray &data, const QString &hash)
{
    Fb2TemporaryFile & file = get(name, hash);
    file.write(data);
    return file.hash();
}

QString Fb2TemporaryList::hash(const QString &path) const
{
    Fb2TemporaryIterator it(*this);
    while (it.hasNext()) {
        Fb2TemporaryFile *file = it.next();
        if (file->fileName() == path) return file->hash();
    }
    return QString();
}

QString Fb2TemporaryList::name(const QString &hash) const
{
    Fb2TemporaryIterator it(*this);
    while (it.hasNext()) {
        Fb2TemporaryFile *file = it.next();
        if (file->hash() == hash) return file->name();
    }
    return QString();
}

QString Fb2TemporaryList::data(const QString &name) const
{
    Fb2TemporaryIterator it(*this);
    while (it.hasNext()) {
        Fb2TemporaryFile *file = it.next();
        if (file->name() == name) return file->readAll().toBase64();
    }
    return QString();
}

bool Fb2TemporaryList::exists(const QString &name) const
{
    Fb2TemporaryIterator it(*this);
    while (it.hasNext()) {
        Fb2TemporaryFile *file = it.next();
        if (file->name() == name) return true;
    }
    return false;
}

#if 0

//---------------------------------------------------------------------------
//  Fb2NetworkDiskCache
//---------------------------------------------------------------------------

QNetworkCacheMetaData Fb2NetworkDiskCache::metaData(const QUrl &url)
{
    qCritical() << url.toString();
    return QNetworkDiskCache::metaData(url);
}

QIODevice * Fb2NetworkDiskCache::data(const QUrl &url)
{
    qCritical() << url.toString();
    return QNetworkDiskCache::data(url);
}

#endif

//---------------------------------------------------------------------------
//  Fb2ImageReply
//---------------------------------------------------------------------------

Fb2ImageReply::Fb2ImageReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request, const QByteArray &data)
    : QNetworkReply()
    , content(data)
    , offset(0)
{
    setOperation(op);
    setRequest(request);
    setUrl(request.url());
    open(ReadOnly | Unbuffered);
    setHeader(QNetworkRequest::ContentLengthHeader, QVariant(content.size()));
    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}

qint64 Fb2ImageReply::readData(char *data, qint64 maxSize)
{
    if (offset >= content.size()) return -1;
    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    qint64 number = qMin(maxSize, content.size() - offset);
    memcpy(data, content.constData() + offset, number);
    offset += number;
    return number;
}

//---------------------------------------------------------------------------
//  Fb2NetworkAccessManager
//
//    http://doc.trolltech.com/qq/32/qq32-webkit-protocols.html
//---------------------------------------------------------------------------

Fb2NetworkAccessManager::Fb2NetworkAccessManager(Fb2WebView &view)
    : QNetworkAccessManager(&view)
    , m_view(view)
{
}

QNetworkReply * Fb2NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    if (request.url().scheme() == "fb2") return imageRequest(op, request);
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

QNetworkReply * Fb2NetworkAccessManager::imageRequest(Operation op, const QNetworkRequest &request)
{
    QString name = request.url().path();
    QByteArray data = m_view.files().fileData(name);
    return new Fb2ImageReply(op, request, data);
}
