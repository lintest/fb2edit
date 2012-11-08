#ifndef FB2LOGS_H
#define FB2LOGS_H

#include <QScopedPointer>
#include <QXmlParseException>

class FbMessagePrivate;

class FbMessage : public QObject
{
    Q_OBJECT

public:
    enum Level {
        Message,
        Warring,
        Error,
        Fatal
    };

public:
    explicit FbMessage();
    FbMessage(const FbMessage &other);
    FbMessage(const QXmlParseException &error, Level level = Message);
    ~FbMessage();

    QString msg() const;
    int level() const;
    int row() const;
    int col() const;

private:
    QScopedPointer<FbMessagePrivate> d;

};

#endif // FB2LOGS_H
