#include "fb2doc.h"
#include "fb2read.h"

Fb2MainDocument * Fb2MainDocument::load(QIODevice &io)
{
    Fb2MainDocument * document = new Fb2MainDocument;

    Fb2Handler handler(*document);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    QXmlInputSource source(&io);

    if (reader.parse(source)) {
        return document;
    } else {
        delete document;
        return NULL;
    }
}
