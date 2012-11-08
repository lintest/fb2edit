#include "fb2logs.hpp"

//---------------------------------------------------------------------------
//  FbMessage
//---------------------------------------------------------------------------

class FbMessagePrivate
{
public:
    FbMessagePrivate()
        : level(FbMessage::Message), row(-1), col(-1)
    {
    }

    FbMessagePrivate(const FbMessagePrivate &other)
        : level(other.level), msg(other.msg), row(other.row), col(other.col)
    {
    }
private:
    friend class FbMessage;
    FbMessage::Level level;
    QString msg;
    int row;
    int col;
};

FbMessage::FbMessage()
    : d(new FbMessagePrivate)
{
}

FbMessage::FbMessage(const FbMessage &other)
    : d(new FbMessagePrivate(*other.d))
{
}

FbMessage::FbMessage(const QXmlParseException &error, Level level)
    : d(new FbMessagePrivate)
{
    d->level = level;
    d->msg = error.message().simplified();
    d->row = error.lineNumber();
    d->col = error.columnNumber();
}

FbMessage::~FbMessage()
{
}

QString FbMessage::msg() const
{
    return d->msg;
}

int FbMessage::level() const
{
    return d->level;
}

int FbMessage::row() const
{
    return d->row;
}

int FbMessage::col() const
{
    return d->col;
}
