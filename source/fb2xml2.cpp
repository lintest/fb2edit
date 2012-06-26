#include "fb2xml2.h"

#ifdef FB2_USE_LIBXML2

#include <cstring>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#include <libxml/xmlreader.h>
#include <QtDebug>

namespace XML2 {

//---------------------------------------------------------------------------
//  XML2::HtmlReader
//---------------------------------------------------------------------------

class HtmlReaderLocator : public QXmlLocator {
public:
    HtmlReaderLocator(HtmlReader* r) : reader(r) {}
    virtual int columnNumber(void) const;
    virtual int lineNumber(void) const;
private:
    HtmlReader* reader;
};

class HtmlReaderPrivate
{
private:
    class ClosedTag : public QList<QString> { public: ClosedTag(); };
public:
    ~HtmlReaderPrivate(void) {}
private:
    HtmlReaderPrivate(HtmlReader* reader);

    static void startDocument(void* c);
    static void endDocument(void* c);
    static void startElement(void* c, const xmlChar* name, const xmlChar** attrs);
    static void endElement(void* c, const xmlChar* name);
    static void comment(void* c, const xmlChar* value);
    static void cdataBlock(void* c, const xmlChar* value, int len);
    static void processingInstruction(void* c, const xmlChar* target, const xmlChar* data);
    static void characters(void* c, const xmlChar* ch, int len);
    static void ignorableWhitespace(void* c, const xmlChar* ch, int len);
    static void internalSubset(void* c, const xmlChar* name, const xmlChar* publicId, const xmlChar* systemId);

    static QString C2S(const xmlChar* text, int size = -1);
    static QString local(const QString &name);

    void parse(const QXmlInputSource* input);

    QScopedPointer<HtmlReaderLocator> locator;
    Q_DECLARE_PUBLIC(HtmlReader)
    HtmlReader* q_ptr;

    QXmlEntityResolver* entityresolver;
    QXmlDTDHandler*     dtdhandler;
    QXmlContentHandler* contenthandler;
    QXmlErrorHandler*   errorhandler;
    QXmlLexicalHandler* lexicalhandler;
    QXmlDeclHandler*    declhandler;

    xmlParserCtxt* context;
    QList<QString> closed;

    friend class HtmlReaderLocator;
};

HtmlReaderPrivate::HtmlReaderPrivate(HtmlReader* reader)
    : q_ptr(reader), entityresolver(0), dtdhandler(0), contenthandler(0), errorhandler(0), lexicalhandler(0), declhandler(0), context(0)
{
    this->locator.reset(new HtmlReaderLocator(reader));
}

HtmlReaderPrivate::ClosedTag::ClosedTag()
{
    *this << "area";
    *this << "base";
    *this << "br";
    *this << "col";
    *this << "command";
    *this << "embed";
    *this << "hr";
    *this << "img";
    *this << "input";
    *this << "keygen";
    *this << "link";
    *this << "meta";
    *this << "param";
    *this << "source";
    *this << "track";
    *this << "wbr";
}

QString HtmlReaderPrivate::C2S(const xmlChar* text, int size)
{
    return QString::fromLocal8Bit(reinterpret_cast<const char*>(text), size);
}

void HtmlReaderPrivate::parse(const QXmlInputSource* input)
{
    htmlSAXHandler handler;
    QByteArray arr = input->data().toUtf8();

    std::memset(&handler, 0, sizeof(handler));
    handler.startDocument         = &HtmlReaderPrivate::startDocument;
    handler.endDocument           = &HtmlReaderPrivate::endDocument;
    handler.startElement          = &HtmlReaderPrivate::startElement;
    handler.endElement            = &HtmlReaderPrivate::endElement;
    handler.comment               = &HtmlReaderPrivate::comment;
    handler.cdataBlock            = &HtmlReaderPrivate::cdataBlock;
    handler.processingInstruction = &HtmlReaderPrivate::processingInstruction;
    handler.characters            = &HtmlReaderPrivate::characters;
    handler.ignorableWhitespace   = &HtmlReaderPrivate::ignorableWhitespace;
    handler.internalSubset        = &HtmlReaderPrivate::internalSubset;

    this->context = htmlCreatePushParserCtxt(&handler, this, arr.constData(), arr.size(), "", XML_CHAR_ENCODING_UTF8);
    htmlParseChunk(this->context, NULL, 0, 1);
    htmlFreeParserCtxt(this->context);
    xmlCleanupParser();
}

void HtmlReaderPrivate::startDocument(void* c)
{
    HtmlReaderPrivate* r = reinterpret_cast<HtmlReaderPrivate*>(c);
    if (r->contenthandler) {
        r->contenthandler->startDocument();
    }
}

void HtmlReaderPrivate::endDocument(void* c)
{
    HtmlReaderPrivate* r = reinterpret_cast<HtmlReaderPrivate*>(c);
    if (r->contenthandler) {
        r->contenthandler->endDocument();
    }
}

QString HtmlReaderPrivate::local(const QString &name)
{
    return name.mid(name.lastIndexOf(":"));
}

void HtmlReaderPrivate::startElement(void* c, const xmlChar* name, const xmlChar** attrs)
{
    HtmlReaderPrivate* r = reinterpret_cast<HtmlReaderPrivate*>(c);
    if (r->contenthandler) {
        QXmlAttributes a;
        if (attrs) {
            int i = 0;
            while (attrs[i]) {
                QString qName = C2S(attrs[i]);
                a.append(qName, "", local(qName), C2S(attrs[i+1]));
                i += 2;
            }
        }
        static ClosedTag closed;
        QString qName = C2S(name);
        QString localName = local(qName);
        r->contenthandler->startElement("", localName, qName, a);
        if (closed.indexOf(qName.toLower()) != -1) {
            r->contenthandler->endElement("", localName, qName);
        }
    }
}

void HtmlReaderPrivate::endElement(void* c, const xmlChar* name)
{
    HtmlReaderPrivate* r = reinterpret_cast<HtmlReaderPrivate*>(c);
    if (r->contenthandler) {
        QString qName = C2S(name);
        r->contenthandler->endElement("", local(qName), qName);
    }
}

void HtmlReaderPrivate::comment(void* c, const xmlChar* value)
{
    HtmlReaderPrivate* r = reinterpret_cast<HtmlReaderPrivate*>(c);
    if (r->lexicalhandler) {
        r->lexicalhandler->comment(C2S(value));
    }
}

void HtmlReaderPrivate::cdataBlock(void* c, const xmlChar* value, int len)
{
    HtmlReaderPrivate* r = reinterpret_cast<HtmlReaderPrivate*>(c);
    if (r->lexicalhandler) {
        r->lexicalhandler->startCDATA();
        if (r->contenthandler) {
            r->contenthandler->characters(C2S(value, len));
        }
        r->lexicalhandler->endCDATA();
    }
}

void HtmlReaderPrivate::processingInstruction(void* c, const xmlChar* target, const xmlChar* data)
{
    HtmlReaderPrivate* r = reinterpret_cast<HtmlReaderPrivate*>(c);
    if (r->contenthandler) {
        r->contenthandler->processingInstruction(C2S(target), C2S(data));
    }
}

void HtmlReaderPrivate::characters(void* c, const xmlChar* ch, int len)
{
    HtmlReaderPrivate* r = reinterpret_cast<HtmlReaderPrivate*>(c);
    if (r->contenthandler) {
        r->contenthandler->characters(C2S(ch, len));
    }
}

void HtmlReaderPrivate::ignorableWhitespace(void* c, const xmlChar* ch, int len)
{
    HtmlReaderPrivate* r = reinterpret_cast<HtmlReaderPrivate*>(c);
    if (r->contenthandler) {
        r->contenthandler->ignorableWhitespace(C2S(ch, len));
    }
}

void HtmlReaderPrivate::internalSubset(void* c, const xmlChar* name, const xmlChar* publicId, const xmlChar* systemId)
{
    HtmlReaderPrivate* r = reinterpret_cast<HtmlReaderPrivate*>(c);
    if (r->lexicalhandler) {
        r->lexicalhandler->startDTD(C2S(name), C2S(publicId), C2S(systemId));
        r->lexicalhandler->endDTD();
    }

}

HtmlReader::HtmlReader(void)
    : d_ptr(new HtmlReaderPrivate(this))
{
}

HtmlReader::~HtmlReader(void)
{
}

bool HtmlReader::feature(const QString&, bool* ok) const
{
    if (ok) {
        *ok = false;
    }

    return false;
}

void HtmlReader::setFeature(const QString&, bool)
{
}

bool HtmlReader::hasFeature(const QString&) const
{
    return false;
}

void* HtmlReader::property(const QString&, bool* ok) const
{
    if (ok) {
        *ok = false;
    }

    return 0;
}

void HtmlReader::setProperty(const QString&, void*)
{
}

bool HtmlReader::hasProperty(const QString&) const
{
    return false;
}

void HtmlReader::setEntityResolver(QXmlEntityResolver* handler)
{
    Q_D(HtmlReader);
    d->entityresolver = handler;
}

QXmlEntityResolver* HtmlReader::entityResolver(void) const
{
    const HtmlReaderPrivate* d = this->d_func();
    return d->entityresolver;
}

void HtmlReader::setDTDHandler(QXmlDTDHandler* handler)
{
    Q_D(HtmlReader);
    d->dtdhandler = handler;
}

QXmlDTDHandler* HtmlReader::DTDHandler(void) const
{
    const HtmlReaderPrivate* d = this->d_func();
    return d->dtdhandler;
}

void HtmlReader::setContentHandler(QXmlContentHandler* handler)
{
    Q_D(HtmlReader);
    d->contenthandler = handler;
}

QXmlContentHandler* HtmlReader::contentHandler(void) const
{
    const HtmlReaderPrivate* d = this->d_func();
    return d->contenthandler;
}

void HtmlReader::setErrorHandler(QXmlErrorHandler* handler)
{
    Q_D(HtmlReader);
    d->errorhandler = handler;
}

QXmlErrorHandler* HtmlReader::errorHandler(void) const
{
    const HtmlReaderPrivate* d = this->d_func();
    return d->errorhandler;
}

void HtmlReader::setLexicalHandler(QXmlLexicalHandler* handler)
{
    Q_D(HtmlReader);
    d->lexicalhandler = handler;
}

QXmlLexicalHandler* HtmlReader::lexicalHandler(void) const
{
    const HtmlReaderPrivate* d = this->d_func();
    return d->lexicalhandler;
}

void HtmlReader::setDeclHandler(QXmlDeclHandler* handler)
{
    Q_D(HtmlReader);
    d->declhandler = handler;
}

QXmlDeclHandler* HtmlReader::declHandler(void) const
{
    const HtmlReaderPrivate* d = this->d_func();
    return d->declhandler;
}

bool HtmlReader::parse(const QXmlInputSource& input)
{
    return this->parse(&input);
}

bool HtmlReader::parse(const QXmlInputSource* input)
{
    Q_D(HtmlReader);

    if (d->contenthandler) {
        d->contenthandler->setDocumentLocator(d->locator.data());
    }

    d->parse(input);

    return true;
}

int HtmlReaderLocator::columnNumber(void) const
{
    return this->reader->d_func()->context->input->col;
}

int HtmlReaderLocator::lineNumber(void) const
{
    return this->reader->d_func()->context->input->line;
}

//---------------------------------------------------------------------------
//  XML2::HtmlReader
//---------------------------------------------------------------------------

class XmlReaderLocator : public QXmlLocator {
public:
    XmlReaderLocator(XmlReader* r) : reader(r) {}
    virtual int columnNumber(void) const;
    virtual int lineNumber(void) const;
private:
    XmlReader* reader;
};

class XmlReaderPrivate {
public:
    ~XmlReaderPrivate(void) {}
private:
    XmlReaderPrivate(XmlReader* reader);

    static void onError(void *arg, const char *msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator);
    static int onRead(void * context, char * buffer, int len);

    static QString C2S(const xmlChar* text, int size = -1);

    bool parse(const QXmlInputSource* input);
    bool parse(QIODevice& input);
    void process(xmlTextReaderPtr reader);

    QScopedPointer<XmlReaderLocator> locator;
    Q_DECLARE_PUBLIC(XmlReader)
    XmlReader* q_ptr;

    QXmlEntityResolver* entityresolver;
    QXmlDTDHandler*     dtdhandler;
    QXmlContentHandler* contenthandler;
    QXmlErrorHandler*   errorhandler;
    QXmlLexicalHandler* lexicalhandler;
    QXmlDeclHandler*    declhandler;

    xmlTextReaderPtr m_reader;

    friend class XmlReaderLocator;
};

XmlReaderPrivate::XmlReaderPrivate(XmlReader* reader)
    : q_ptr(reader), entityresolver(0), dtdhandler(0), contenthandler(0), errorhandler(0), lexicalhandler(0), declhandler(0), m_reader(0)
{
    this->locator.reset(new XmlReaderLocator(reader));
}

QString XmlReaderPrivate::C2S(const xmlChar* text, int size)
{
    return QString::fromLocal8Bit(reinterpret_cast<const char*>(text), size);
}

void XmlReaderPrivate::onError(void * arg, const char * msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator)
{
    XmlReaderPrivate* r = reinterpret_cast<XmlReaderPrivate*>(arg);
    if (r->errorhandler) {
        QXmlParseException e(QString::fromLocal8Bit(msg), xmlTextReaderGetParserColumnNumber(r->m_reader), xmlTextReaderGetParserLineNumber(r->m_reader));
        switch (severity) {
            case XML_PARSER_SEVERITY_VALIDITY_WARNING: r->errorhandler->warning(e); break;
            case XML_PARSER_SEVERITY_VALIDITY_ERROR: r->errorhandler->error(e); break;
            case XML_PARSER_SEVERITY_WARNING: r->errorhandler->warning(e); break;
            case XML_PARSER_SEVERITY_ERROR: r->errorhandler->error(e); break;
        }
    }
}

void XmlReaderPrivate::process(xmlTextReaderPtr reader)
{
    if (!contenthandler) return;
    switch (xmlTextReaderNodeType(reader)) {
        case XML_READER_TYPE_ELEMENT: {
            QString localName = C2S(xmlTextReaderConstLocalName(reader));
            QString qName = C2S(xmlTextReaderConstName(reader));
            bool empty = xmlTextReaderIsEmptyElement(reader);
            QXmlAttributes atts;
            while (xmlTextReaderMoveToNextAttribute(reader)) {
                QString localName = C2S(xmlTextReaderConstLocalName(reader));
                QString qName = C2S(xmlTextReaderConstName(reader));
                QString value = C2S(xmlTextReaderConstValue(reader));
                atts.append(qName, "", localName, value);
            }
            contenthandler->startElement("", localName, qName, atts);
            if (empty) contenthandler->endElement("", localName, qName);
        } break;
        case XML_READER_TYPE_TEXT: {
            QString value = C2S(xmlTextReaderConstValue(reader));
            contenthandler->characters(value);
        } break;
        case XML_READER_TYPE_END_ELEMENT: {
            QString localName = C2S(xmlTextReaderConstLocalName(reader));
            QString qName = C2S(xmlTextReaderConstName(reader));
            contenthandler->endElement("", localName, qName);
        } break;
        case XML_READER_TYPE_COMMENT: {
            if (lexicalhandler) {
                QString value = C2S(xmlTextReaderConstValue(reader));
                lexicalhandler->comment(value);
            }
        } break;
    }
}

int XmlReaderPrivate::onRead(void * context, char * buffer, int len)
{
    QIODevice *device = reinterpret_cast<QIODevice*>(context);
    return device->read(buffer, len);
}

bool XmlReaderPrivate::parse(const QXmlInputSource* input)
{
    QByteArray arr = input->data().toUtf8();
    int options = XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET;
    m_reader = xmlReaderForMemory(arr.constData(), arr.size(), NULL, NULL, options);
    if (!m_reader) return false;
    xmlTextReaderSetErrorHandler(m_reader, &XmlReaderPrivate::onError, this);
    while (xmlTextReaderRead(m_reader) == 1) process(m_reader);
    xmlFreeTextReader(m_reader);
    return true;
}

bool XmlReaderPrivate::parse(QIODevice& input)
{
    int options = XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NONET;
    m_reader = xmlReaderForIO(&XmlReaderPrivate::onRead, NULL, &input, NULL, NULL, options);
    if (!m_reader) return false;
    xmlTextReaderSetErrorHandler(m_reader, &XmlReaderPrivate::onError, this);
    while (xmlTextReaderRead(m_reader) == 1) process(m_reader);
    xmlFreeTextReader(m_reader);
    return true;
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

void XmlReader::setEntityResolver(QXmlEntityResolver* handler)
{
    Q_D(XmlReader);
    d->entityresolver = handler;
}

QXmlEntityResolver* XmlReader::entityResolver(void) const
{
    const XmlReaderPrivate* d = this->d_func();
    return d->entityresolver;
}

void XmlReader::setDTDHandler(QXmlDTDHandler* handler)
{
    Q_D(XmlReader);
    d->dtdhandler = handler;
}

QXmlDTDHandler* XmlReader::DTDHandler(void) const
{
    const XmlReaderPrivate* d = this->d_func();
    return d->dtdhandler;
}

void XmlReader::setContentHandler(QXmlContentHandler* handler)
{
    Q_D(XmlReader);
    d->contenthandler = handler;
}

QXmlContentHandler* XmlReader::contentHandler(void) const
{
    const XmlReaderPrivate* d = this->d_func();
    return d->contenthandler;
}

void XmlReader::setErrorHandler(QXmlErrorHandler* handler)
{
    Q_D(XmlReader);
    d->errorhandler = handler;
}

QXmlErrorHandler* XmlReader::errorHandler(void) const
{
    const XmlReaderPrivate* d = this->d_func();
    return d->errorhandler;
}

void XmlReader::setLexicalHandler(QXmlLexicalHandler* handler)
{
    Q_D(XmlReader);
    d->lexicalhandler = handler;
}

QXmlLexicalHandler* XmlReader::lexicalHandler(void) const
{
    const XmlReaderPrivate* d = this->d_func();
    return d->lexicalhandler;
}

void XmlReader::setDeclHandler(QXmlDeclHandler* handler)
{
    Q_D(XmlReader);
    d->declhandler = handler;
}

QXmlDeclHandler* XmlReader::declHandler(void) const
{
    const XmlReaderPrivate* d = this->d_func();
    return d->declhandler;
}

bool XmlReader::parse(const QXmlInputSource& input)
{
    return this->parse(&input);
}

bool XmlReader::parse(const QXmlInputSource* input)
{
    Q_D(XmlReader);

    if (d->contenthandler) {
        d->contenthandler->setDocumentLocator(d->locator.data());
    }

    d->parse(input);

    return true;
}

bool XmlReader::parse(QIODevice& input)
{
    Q_D(XmlReader);

    if (d->contenthandler) {
        d->contenthandler->setDocumentLocator(d->locator.data());
    }

    d->parse(input);

    return true;
}

int XmlReaderLocator::columnNumber(void) const
{
    return xmlTextReaderGetParserColumnNumber(this->reader->d_func()->m_reader);
}

int XmlReaderLocator::lineNumber(void) const
{
    return xmlTextReaderGetParserLineNumber(this->reader->d_func()->m_reader);
}

} // namespace XML2

#endif // FB2_USE_LIBXML2
