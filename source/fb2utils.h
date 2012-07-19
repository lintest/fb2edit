#ifndef FB2UTILS_H
#define FB2UTILS_H

#include <QIcon>
#include <QString>

#define FB2DELETE(p) { if ((p) != NULL) { delete (p); (p) = NULL; } }

namespace FB2 {

QIcon icon(const QString &name);

QString read(const QString &filename);

}

#endif // FB2UTILS_H
