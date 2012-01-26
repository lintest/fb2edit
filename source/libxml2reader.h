#ifndef LIBXML2READER_H
#define LIBXML2READER_H

/////////////////////////////////////////////////////////////////////////////
//
//   Append into project file:
//     INCLUDEPATH += /usr/include/libxml2
//     LIBS        += -lxml2
//
//   http://blog.sjinks.pro/c-cpp/qt/942-html-parser-qt/
//
//
//     QByteArray data;
//     QXmlInputSource src;
//     LibXml2Reader reader;
//     QDomDocument doc;
//     src.setData(data);
//     doc.setContent(&src, &reader);
//
/////////////////////////////////////////////////////////////////////////////

#include <QtXml/QXmlReader>
#include <libxml/xmlstring.h>

class LibXml2ReaderPrivate;

class LibXml2Reader : public QXmlReader {
public:
	LibXml2Reader(void);
	virtual ~LibXml2Reader(void);

	virtual bool feature(const QString& name, bool* ok = 0) const;
	virtual void setFeature(const QString& name, bool value);
	virtual bool hasFeature(const QString& name) const;
	virtual void* property(const QString& name, bool* ok = 0) const;
	virtual void setProperty(const QString& name, void* value);
	virtual bool hasProperty(const QString& name) const;

	virtual void setEntityResolver(QXmlEntityResolver* handler);
	virtual QXmlEntityResolver* entityResolver(void) const;
	virtual void setDTDHandler(QXmlDTDHandler* handler);
	virtual QXmlDTDHandler* DTDHandler(void) const;
	virtual void setContentHandler(QXmlContentHandler* handler);
	virtual QXmlContentHandler* contentHandler(void) const;
	virtual void setErrorHandler(QXmlErrorHandler* handler);
	virtual QXmlErrorHandler* errorHandler(void) const;
	virtual void setLexicalHandler(QXmlLexicalHandler* handler);
	virtual QXmlLexicalHandler* lexicalHandler(void) const;
	virtual void setDeclHandler(QXmlDeclHandler* handler);
	virtual QXmlDeclHandler* declHandler(void) const;

	virtual bool parse(const QXmlInputSource& input);
	virtual bool parse(const QXmlInputSource* input);


private:
	Q_DISABLE_COPY(LibXml2Reader)
	Q_DECLARE_PRIVATE(LibXml2Reader)
	QScopedPointer<LibXml2ReaderPrivate> d_ptr;

	friend class LibXml2ReaderLocator;
};

#endif // LIBXML2READER_H
