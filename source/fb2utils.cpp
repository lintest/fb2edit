#include "fb2utils.h"

#include <QFile>
#include <QTextStream>

namespace FB2 {

QIcon icon(const QString &name)
{
    QIcon icon;
    icon.addFile(QString(":/24x24/%1.png").arg(name), QSize(24,24));
    icon.addFile(QString(":/16x24/%1.png").arg(name), QSize(16,16));
    return QIcon::fromTheme(name, icon);
}

QString read(const QString &filename)
{
    // TODO: throw an exception instead of
    // returning an empty string
    QFile file( filename );
    if (!file.open( QFile::ReadOnly)) return QString();

    QTextStream in( &file );

    // Input should be UTF-8
    in.setCodec( "UTF-8" );

    // This will automatically switch reading from
    // UTF-8 to UTF-16 if a BOM is detected
    in.setAutoDetectUnicode( true );

    return in.readAll();
}

}
