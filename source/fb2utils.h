#ifndef FB2TOOL_H
#define FB2TOOL_H

#include <QIcon>
#include <QString>

QT_BEGIN_NAMESPACE
class QToolBar;
class QWebView;
QT_END_NAMESPACE


#define FB2DELETE(p) { if ((p) != NULL) { delete (p); (p) = NULL; } }

namespace FB2 {

QIcon icon(const QString &name);

QString read(const QString &filename);

void addTools(QToolBar *tool, QWebView *view);

}

#endif // FB2TOOL_H
