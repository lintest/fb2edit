#include "fb2temp.hpp"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QImageReader>
#include <QUrl>
#include <QtDebug>

#include "fb2view.hpp"

//---------------------------------------------------------------------------
//  Fb2TemporaryFile
//---------------------------------------------------------------------------

Fb2TemporaryFile::Fb2TemporaryFile(const QString &name)
    : QTemporaryFile()
    , m_name(name)
{
}

qint64 Fb2TemporaryFile::write(const QByteArray &data)
{
    open();
    if (m_hash.isEmpty()) m_hash = md5(data);
    qint64 size = QTemporaryFile::write(data);
    close();

    return size;
}

QString Fb2TemporaryFile::md5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toBase64();
}

QByteArray Fb2TemporaryFile::data()
{
    open();
    QByteArray data = readAll();
    close();
    return data;
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

QString Fb2TemporaryList::add(const QString &path, const QByteArray &data)
{
    QString hash = Fb2TemporaryFile::md5(data);
    QString name = this->name(hash);
    if (name.isEmpty()) {
        name = newName(path);
        Fb2TemporaryFile * temp = new Fb2TemporaryFile(name);
        temp->setHash(hash);
        temp->write(data);
        append(temp);
    }
    return name;
}

QString Fb2TemporaryList::newName(const QString &path)
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

Fb2TemporaryFile * Fb2TemporaryList::get(const QString &name) const
{
    Fb2TemporaryIterator it(*this);
    while (it.hasNext()) {
        Fb2TemporaryFile * file = it.next();
        if (file->name() == name) return file;
    }
    return NULL;
}

QByteArray Fb2TemporaryList::data(const QString &name) const
{
    Fb2TemporaryIterator it(*this);
    while (it.hasNext()) {
        Fb2TemporaryFile *file = it.next();
        if (file->name() == name) return file->data();
    }
    return QByteArray();
}

const QString & Fb2TemporaryList::set(const QString &name, const QByteArray &data, const QString &hash)
{
    Fb2TemporaryFile * file = get(name);
    if (!file) append(file = new Fb2TemporaryFile(name));
    file->setHash(hash);
    file->write(data);
    return file->hash();
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
    setAttribute(QNetworkRequest::CacheSaveControlAttribute, QVariant(false));
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

Fb2NetworkAccessManager::Fb2NetworkAccessManager(Fb2TextEdit &view)
    : QNetworkAccessManager(&view)
    , m_view(view)
{
}

QNetworkReply * Fb2NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    if (request.url().scheme() == "fb2" && request.url().path() == m_view.url().path()) return imageRequest(op, request);
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

QNetworkReply * Fb2NetworkAccessManager::imageRequest(Operation op, const QNetworkRequest &request)
{
    QString name = request.url().fragment();
    QByteArray data = m_view.files().data(name);
    return new Fb2ImageReply(op, request, data);
}
