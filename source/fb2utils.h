#ifndef FB2TOOL_H
#define FB2TOOL_H

#include <QString>

#define FB2DELETE(p) { if ((p) != NULL) { delete (p); (p) = NULL; } }

namespace FB2 {

QString read(const QString &filename);

}

#endif // FB2TOOL_H
