#ifndef FB2XML2_H
#define FB2XML2_H

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
//     HtmlReader reader;
//     QDomDocument doc;
//     src.setData(data);
//     doc.setContent(&src, &reader);
//
/////////////////////////////////////////////////////////////////////////////

#include <QtXml/QXmlReader>
#include <libxml/xmlstring.h>

namespace XML2 {

class HtmlReaderPrivate;

class HtmlReader : public QXmlReader {
public:
    HtmlReader(void);
    virtual ~HtmlReader(void);

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
    Q_DISABLE_COPY(HtmlReader)
    Q_DECLARE_PRIVATE(HtmlReader)
    QScopedPointer<HtmlReaderPrivate> d_ptr;

    friend class HtmlReaderLocator;
};

class XmlReaderPrivate;

class XmlReader : public QXmlReader {
public:
    XmlReader(void);
    virtual ~XmlReader(void);

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

    virtual bool parse(QIODevice& input);
    virtual bool parse(const QXmlInputSource&) { return false; }
    virtual bool parse(const QXmlInputSource*) { return false; }


private:
    Q_DISABLE_COPY(XmlReader)
    Q_DECLARE_PRIVATE(XmlReader)
    QScopedPointer<XmlReaderPrivate> d_ptr;

    friend class XmlReaderLocator;
};

} // namespace XML2

#endif // FB2XML2_H
