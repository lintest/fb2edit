#ifndef FB2TEMP_H
#define FB2TEMP_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QTemporaryFile>

class Fb2TemporaryFile : public QTemporaryFile
{
    Q_OBJECT
public:
    explicit Fb2TemporaryFile(const QString &name, const QString &hash = QString());
    inline qint64 write(const QByteArray &data);
    const QString & hash() const { return m_hash; }
    const QString & name() const { return m_name; }
    static QString md5(const QByteArray &data);
private:
    const QString m_name;
    QString m_hash;
};

class Fb2TemporaryList : public QList<Fb2TemporaryFile*>
{
public:
    explicit Fb2TemporaryList();
    virtual ~Fb2TemporaryList();

    bool exists(const QString &name) const;
    Fb2TemporaryFile & get(const QString &name, const QString &hash = QString());
    QString set(const QString &name, const QByteArray &data, const QString &hash = QString());

    QString hash(const QString &path) const;
    QString name(const QString &hash) const;
    QString data(const QString &name) const;
};

typedef QListIterator<Fb2TemporaryFile*> Fb2TemporaryIterator;

#endif // FB2TEMP_H
