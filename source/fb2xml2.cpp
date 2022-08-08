#include "fb2xml2.h"

#ifdef FB2_USE_LIBXML2

#include <cstring>
#include <QtDebug>

namespace XML2 {

//---------------------------------------------------------------------------
//  XML2::XmlReader
//---------------------------------------------------------------------------

class XmlReaderPrivate {
public:
    ~XmlReaderPrivate(void) {}
private:
    XmlReaderPrivate(XmlReader* reader);

    bool parse(const QXmlInputSource *input);
    bool parse(QIODevice *input);
    bool process(QXmlStreamReader& reader);

    Q_DECLARE_PUBLIC(XmlReader)
    XmlReader* q_ptr;

    FbXmlHandler* contenthandler;
    FbXmlHandler* errorhandler;
    FbXmlHandler* lexicalhandler;
};

XmlReaderPrivate::XmlReaderPrivate(XmlReader* reader)
    : q_ptr(reader)
    , contenthandler(nullptr)
    , errorhandler(nullptr)
    , lexicalhandler(nullptr)
{
}

bool XmlReaderPrivate::process(QXmlStreamReader &reader)
{
    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.hasError()) {
            return errorhandler->error(reader.errorString(), reader.lineNumber(), reader.columnNumber());
        }

        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (!contenthandler->startElement(reader.namespaceUri().toString(), reader.name().toString(),
                                              reader.qualifiedName().toString(), reader.attributes())) {
                return false;
            }
            break;
        case QXmlStreamReader::EndElement:
            if (!contenthandler->endElement(reader.namespaceUri().toString(), reader.name().toString(),
                                            reader.qualifiedName().toString())) {
                return false;
            }
        case QXmlStreamReader::Characters:
            if (!contenthandler->characters(reader.text().toString())) {
                return false;
            }
            break;
        case QXmlStreamReader::Comment:
            if (lexicalhandler && !lexicalhandler->comment(reader.text().toString())) {
                return false;
            }
            break;
        default:
            break;
        }
    }

    return !reader.isEndDocument();
}

bool XmlReaderPrivate::parse(const QXmlInputSource *input)
{
    QXmlStreamReader reader(input->data().toUtf8());

    return process(reader);
}

bool XmlReaderPrivate::parse(QIODevice *input)
{
    QXmlStreamReader reader(input);

    return process(reader);
}

XmlReader::XmlReader(void)
    : d_ptr(new XmlReaderPrivate(this))
{
}

XmlReader::~XmlReader(void)
{
}

bool XmlReader::feature(const QString&, bool* ok) const
{
    if (ok) *ok = false;
    return false;
}

void XmlReader::setFeature(const QString&, bool)
{
}

bool XmlReader::hasFeature(const QString&) const
{
    return false;
}

void* XmlReader::property(const QString&, bool* ok) const
{
    if (ok) *ok = false;
    return 0;
}

void XmlReader::setProperty(const QString&, void*)
{
}

bool XmlReader::hasProperty(const QString&) const
{
    return false;
}

void XmlReader::setContentHandler(FbXmlHandler* handler)
{
    Q_D(XmlReader);
    d->contenthandler = handler;
}

FbXmlHandler* XmlReader::contentHandler(void) const
{
    const XmlReaderPrivate* d = this->d_func();
    return d->contenthandler;
}

void XmlReader::setErrorHandler(FbXmlHandler* handler)
{
    Q_D(XmlReader);
    d->errorhandler = handler;
}

FbXmlHandler* XmlReader::errorHandler(void) const
{
    const XmlReaderPrivate* d = this->d_func();
    return d->errorhandler;
}

void XmlReader::setLexicalHandler(FbXmlHandler* handler)
{
    Q_D(XmlReader);
    d->lexicalhandler = handler;
}

FbXmlHandler* XmlReader::lexicalHandler(void) const
{
    const XmlReaderPrivate* d = this->d_func();
    return d->lexicalhandler;
}

bool XmlReader::parse(const QXmlInputSource& input)
{
    return this->parse(&input);
}

bool XmlReader::parse(const QXmlInputSource* input)
{
    Q_D(XmlReader);

    d->parse(input);

    return true;
}

bool XmlReader::parse(QIODevice *input)
{
    Q_D(XmlReader);

    d->parse(input);

    return true;
}

} // namespace XML2

#endif // FB2_USE_LIBXML2
