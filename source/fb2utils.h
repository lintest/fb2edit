#ifndef FB2UTILS_H
#define FB2UTILS_H

#include <QIcon>
#include <QString>

#define FB2DELETE(p) { if ((p) != NULL) { delete (p); (p) = NULL; } }

class FbIcon : public QIcon
{
public:
    explicit FbIcon(const QString &name);
};

QString jScript(const QString &filename);

#endif // FB2UTILS_H
