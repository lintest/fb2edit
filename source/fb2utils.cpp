#include "fb2utils.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

static QIcon loadIcon(const QString &name)
{
    QIcon icon;
    icon.addFile(QString(":/24x24/%1.png").arg(name), QSize(24,24));
    icon.addFile(QString(":/16x16/%1.png").arg(name), QSize(16,16));
    return icon;
}

FbIcon::FbIcon(const QString &name)
    : QIcon(fromTheme(name, loadIcon(name)))
{
}

QString jScript(const QString &filename)
{

#ifdef QT_DEBUG
    QString filepath = qApp->arguments().first();
    filepath += "/../../fb2edit/source/js/";
    filepath += filename;
    filepath = QDir::cleanPath(filepath);
#else
    QString filepath = ":/js/";
    filepath += filename;
#endif

    // TODO: throw an exception instead of
    // returning an empty string
    QFile file( filepath );
    if (!file.open(QFile::ReadOnly)) return QString();

    QTextStream in( &file );

    // Input should be UTF-8
    in.setCodec( "UTF-8" );

    // This will automatically switch reading from
    // UTF-8 to UTF-16 if a BOM is detected
    in.setAutoDetectUnicode( true );

    return in.readAll();
}
