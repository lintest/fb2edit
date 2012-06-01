#include "fb2utils.h"

#include <QFile>
#include <QTextStream>

namespace FB2 {

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
