#include "fb2temp.hpp"

#include <QCryptographicHash>

//---------------------------------------------------------------------------
//  Fb2TemporaryFile
//---------------------------------------------------------------------------

Fb2TemporaryFile::Fb2TemporaryFile(const QString &name)
    : QTemporaryFile()
    , m_name(name)
{
    open();
}

qint64 Fb2TemporaryFile::write(const QByteArray &data)
{
    m_hash = QCryptographicHash::hash(data, QCryptographicHash::Md5).toBase64();
    qint64 size = QTemporaryFile::write(data);
    close();
    open();
    return size;
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

Fb2TemporaryFile & Fb2TemporaryList::get(const QString &name)
{
    Fb2TemporaryIterator it(*this);
    while (it.hasNext()) {
        Fb2TemporaryFile *file = it.next();
        if (file->name() == name) return *file;
    }
    Fb2TemporaryFile * file = new Fb2TemporaryFile(name);
    append(file);
    return *file;
}

void Fb2TemporaryList::set(const QString &name, const QByteArray &data)
{
    get(name).write(data);
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

bool Fb2TemporaryList::exists(const QString &name)
{
    Fb2TemporaryIterator it(*this);
    while (it.hasNext()) {
        Fb2TemporaryFile *file = it.next();
        if (file->name() == name) return true;
    }
    return false;
}
