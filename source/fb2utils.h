#ifndef FB2UTILS_H
#define FB2UTILS_H

#include <QIcon>
#include <QString>

#define FB2DELETE(p) { if ((p) != NULL) { delete (p); (p) = NULL; } }

class Fb2Icon : public QIcon
{
public:
    explicit Fb2Icon(const QString &name);
};

namespace FB2 {

QString read(const QString &filename);

}

#endif // FB2UTILS_H
