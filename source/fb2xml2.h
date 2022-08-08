#ifndef FB2XML2_H
#define FB2XML2_H

#ifdef FB2_USE_LIBXML2

#include <QtXml>
#include "fb2xml.hpp"

namespace XML2 {

class XmlReaderPrivate;

class XmlReader
{
public:
    XmlReader(void);
    ~XmlReader(void);

    bool feature(const QString& name, bool* ok = 0) const;
    void setFeature(const QString& name, bool value);
    bool hasFeature(const QString& name) const;
    void* property(const QString& name, bool* ok = 0) const;
    void setProperty(const QString& name, void* value);
    bool hasProperty(const QString& name) const;

    void setContentHandler(FbXmlHandler* handler);
    FbXmlHandler* contentHandler(void) const;
    void setErrorHandler(FbXmlHandler* handler);
    FbXmlHandler* errorHandler(void) const;
    void setLexicalHandler(FbXmlHandler* handler);
    FbXmlHandler* lexicalHandler(void) const;

    bool parse(QIODevice *input);
    bool parse(const QString&);
    bool parse(const QString*);


private:
    Q_DISABLE_COPY(XmlReader)
    Q_DECLARE_PRIVATE(XmlReader)
    QScopedPointer<XmlReaderPrivate> d_ptr;
};

} // namespace XML2

#endif // FB2_USE_LIBXML2

#endif // FB2XML2_H
