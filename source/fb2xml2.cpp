#include "fb2xml2.h"
#include <cstring>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>

class LibXml2ReaderLocator : public QXmlLocator {
public:
	LibXml2ReaderLocator(LibXml2Reader* r) : reader(r) {}
	virtual int columnNumber(void) const;
	virtual int lineNumber(void) const;
private:
	LibXml2Reader* reader;
};

class LibXml2ReaderPrivate {
public:
	~LibXml2ReaderPrivate(void) {}
private:
	LibXml2ReaderPrivate(LibXml2Reader* reader);

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

	void parse(const QXmlInputSource* input);

	QScopedPointer<LibXml2ReaderLocator> locator;
	Q_DECLARE_PUBLIC(LibXml2Reader)
	LibXml2Reader* q_ptr;

	QXmlEntityResolver* entityresolver;
	QXmlDTDHandler*     dtdhandler;
	QXmlContentHandler* contenthandler;
	QXmlErrorHandler*   errorhandler;
	QXmlLexicalHandler* lexicalhandler;
	QXmlDeclHandler*    declhandler;

	xmlParserCtxt* context;

	friend class LibXml2ReaderLocator;
};

LibXml2ReaderPrivate::LibXml2ReaderPrivate(LibXml2Reader* reader)
	: q_ptr(reader), entityresolver(0), dtdhandler(0), contenthandler(0), errorhandler(0), lexicalhandler(0), declhandler(0), context(0)
{
	this->locator.reset(new LibXml2ReaderLocator(reader));
}

void LibXml2ReaderPrivate::parse(const QXmlInputSource* input)
{
	htmlSAXHandler handler;
        QByteArray arr = input->data().toUtf8();
	const char* data = arr.data();

	std::memset(&handler, 0, sizeof(handler));
	handler.startDocument         = &LibXml2ReaderPrivate::startDocument;
	handler.endDocument           = &LibXml2ReaderPrivate::endDocument;
	handler.startElement          = &LibXml2ReaderPrivate::startElement;
	handler.endElement            = &LibXml2ReaderPrivate::endElement;
	handler.comment               = &LibXml2ReaderPrivate::comment;
	handler.cdataBlock            = &LibXml2ReaderPrivate::cdataBlock;
	handler.processingInstruction = &LibXml2ReaderPrivate::processingInstruction;
	handler.characters            = &LibXml2ReaderPrivate::characters;
	handler.ignorableWhitespace   = &LibXml2ReaderPrivate::ignorableWhitespace;
	handler.internalSubset        = &LibXml2ReaderPrivate::internalSubset;

        this->context = htmlCreatePushParserCtxt(&handler, this, data, xmlStrlen(reinterpret_cast<const xmlChar*>(data)), "", XML_CHAR_ENCODING_UTF8);
	htmlParseChunk(this->context, NULL, 0, 1);
	htmlFreeParserCtxt(this->context);
	xmlCleanupParser();
}

void LibXml2ReaderPrivate::startDocument(void* c)
{
	LibXml2ReaderPrivate* r = reinterpret_cast<LibXml2ReaderPrivate*>(c);
	if (r->contenthandler) {
		r->contenthandler->startDocument();
	}
}

void LibXml2ReaderPrivate::endDocument(void* c)
{
	LibXml2ReaderPrivate* r = reinterpret_cast<LibXml2ReaderPrivate*>(c);
	if (r->contenthandler) {
		r->contenthandler->endDocument();
	}
}

void LibXml2ReaderPrivate::startElement(void* c, const xmlChar* name, const xmlChar** attrs)
{
	LibXml2ReaderPrivate* r = reinterpret_cast<LibXml2ReaderPrivate*>(c);
	if (r->contenthandler) {
		QXmlAttributes a;

		int i = 0;
		if (attrs) {
			while (attrs[i]) {
				const char* name  = reinterpret_cast<const char*>(attrs[i]);
				const char* value = reinterpret_cast<const char*>(attrs[i+1]);
				i += 2;
				a.append(name, "", "", value ? value : name);
			}
		}

		QString uri = "";
		QString localName = "";
		QString qName = reinterpret_cast<const char*>(name);
		r->contenthandler->startElement(uri, localName, qName, a);
	}
}

void LibXml2ReaderPrivate::endElement(void* c, const xmlChar* name)
{
	LibXml2ReaderPrivate* r = reinterpret_cast<LibXml2ReaderPrivate*>(c);
	if (r->contenthandler) {
		r->contenthandler->endElement(QString(""), QString(""), QString(reinterpret_cast<const char*>(name)));
	}
}

void LibXml2ReaderPrivate::comment(void* c, const xmlChar* value)
{
	LibXml2ReaderPrivate* r = reinterpret_cast<LibXml2ReaderPrivate*>(c);
	if (r->lexicalhandler) {
		r->lexicalhandler->comment(QString::fromLocal8Bit(reinterpret_cast<const char*>(value)));
	}
}

void LibXml2ReaderPrivate::cdataBlock(void* c, const xmlChar* value, int len)
{
	LibXml2ReaderPrivate* r = reinterpret_cast<LibXml2ReaderPrivate*>(c);
	if (r->lexicalhandler) {
		r->lexicalhandler->startCDATA();
		if (r->contenthandler) {
			QByteArray arr(reinterpret_cast<const char*>(value), len);
			r->contenthandler->characters(arr);
		}

		r->lexicalhandler->endCDATA();
	}
}

void LibXml2ReaderPrivate::processingInstruction(void* c, const xmlChar* target, const xmlChar* data)
{
	LibXml2ReaderPrivate* r = reinterpret_cast<LibXml2ReaderPrivate*>(c);
	if (r->contenthandler) {
		r->contenthandler->processingInstruction(reinterpret_cast<const char*>(target), reinterpret_cast<const char*>(data));
	}
}

void LibXml2ReaderPrivate::characters(void* c, const xmlChar* ch, int len)
{
	LibXml2ReaderPrivate* r = reinterpret_cast<LibXml2ReaderPrivate*>(c);
	if (r->contenthandler) {
		r->contenthandler->characters(QString::fromLocal8Bit(reinterpret_cast<const char*>(ch), len));
	}
}

void LibXml2ReaderPrivate::ignorableWhitespace(void* c, const xmlChar* ch, int len)
{
	LibXml2ReaderPrivate* r = reinterpret_cast<LibXml2ReaderPrivate*>(c);
	if (r->contenthandler) {
		r->contenthandler->ignorableWhitespace(QString::fromLocal8Bit(reinterpret_cast<const char*>(ch), len));
	}
}

void LibXml2ReaderPrivate::internalSubset(void* c, const xmlChar* name, const xmlChar* publicId, const xmlChar* systemId)
{
	LibXml2ReaderPrivate* r = reinterpret_cast<LibXml2ReaderPrivate*>(c);
	if (r->lexicalhandler) {
		QString n(QString::fromLocal8Bit(reinterpret_cast<const char*>(name)));
		QString p(QString::fromLocal8Bit(reinterpret_cast<const char*>(publicId)));
		QString s(QString::fromLocal8Bit(reinterpret_cast<const char*>(systemId)));
		r->lexicalhandler->startDTD(n, p, s);
		r->lexicalhandler->endDTD();
	}

}

LibXml2Reader::LibXml2Reader(void)
	: d_ptr(new LibXml2ReaderPrivate(this))
{
}

LibXml2Reader::~LibXml2Reader(void)
{
}

bool LibXml2Reader::feature(const QString&, bool* ok) const
{
	if (ok) {
		*ok = false;
	}

	return false;
}

void LibXml2Reader::setFeature(const QString&, bool)
{
}

bool LibXml2Reader::hasFeature(const QString&) const
{
	return false;
}

void* LibXml2Reader::property(const QString&, bool* ok) const
{
	if (ok) {
		*ok = false;
	}

	return 0;
}

void LibXml2Reader::setProperty(const QString&, void*)
{
}

bool LibXml2Reader::hasProperty(const QString&) const
{
	return false;
}

void LibXml2Reader::setEntityResolver(QXmlEntityResolver* handler)
{
	Q_D(LibXml2Reader);
	d->entityresolver = handler;
}

QXmlEntityResolver* LibXml2Reader::entityResolver(void) const
{
	const LibXml2ReaderPrivate* d = this->d_func();
	return d->entityresolver;
}

void LibXml2Reader::setDTDHandler(QXmlDTDHandler* handler)
{
	Q_D(LibXml2Reader);
	d->dtdhandler = handler;
}

QXmlDTDHandler* LibXml2Reader::DTDHandler(void) const
{
	const LibXml2ReaderPrivate* d = this->d_func();
	return d->dtdhandler;
}

void LibXml2Reader::setContentHandler(QXmlContentHandler* handler)
{
	Q_D(LibXml2Reader);
	d->contenthandler = handler;
}

QXmlContentHandler* LibXml2Reader::contentHandler(void) const
{
	const LibXml2ReaderPrivate* d = this->d_func();
	return d->contenthandler;
}

void LibXml2Reader::setErrorHandler(QXmlErrorHandler* handler)
{
	Q_D(LibXml2Reader);
	d->errorhandler = handler;
}

QXmlErrorHandler* LibXml2Reader::errorHandler(void) const
{
	const LibXml2ReaderPrivate* d = this->d_func();
	return d->errorhandler;
}

void LibXml2Reader::setLexicalHandler(QXmlLexicalHandler* handler)
{
	Q_D(LibXml2Reader);
	d->lexicalhandler = handler;
}

QXmlLexicalHandler* LibXml2Reader::lexicalHandler(void) const
{
	const LibXml2ReaderPrivate* d = this->d_func();
	return d->lexicalhandler;
}

void LibXml2Reader::setDeclHandler(QXmlDeclHandler* handler)
{
	Q_D(LibXml2Reader);
	d->declhandler = handler;
}

QXmlDeclHandler* LibXml2Reader::declHandler(void) const
{
	const LibXml2ReaderPrivate* d = this->d_func();
	return d->declhandler;
}

bool LibXml2Reader::parse(const QXmlInputSource& input)
{
	return this->parse(&input);
}

bool LibXml2Reader::parse(const QXmlInputSource* input)
{
	Q_D(LibXml2Reader);

	if (d->contenthandler) {
		d->contenthandler->setDocumentLocator(d->locator.data());
	}

	d->parse(input);

	return true;
}

int LibXml2ReaderLocator::columnNumber(void) const
{
	return this->reader->d_func()->context->input->col;
}

int LibXml2ReaderLocator::lineNumber(void) const
{
	return this->reader->d_func()->context->input->line;
}
