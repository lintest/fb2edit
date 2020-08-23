#include "fb2xml.hpp"
#include <QtDebug>

//---------------------------------------------------------------------------
//  FbXmlHandler::NodeHandler
//---------------------------------------------------------------------------

QString FbXmlHandler::NodeHandler::Value(const QXmlAttributes &attributes, const QString &name)
{
    int count = attributes.count();
    for (int i = 0; i < count; ++i ) {
        if (attributes.localName(i).compare(name, Qt::CaseInsensitive) == 0) {
            return attributes.value(i);
        }
    }
    return QString();
}

bool FbXmlHandler::NodeHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);
    m_handler = NewTag(name, attributes);
    if (m_handler) {
        if (m_handler->m_closed) {
            delete m_handler;
            m_handler = NULL;
        }
        return true;
    }
    m_handler = new NodeHandler(name);
    return true;
}

bool FbXmlHandler::NodeHandler::doText(const QString &text)
{
    if (m_handler) m_handler->doText(text); else TxtTag(text);
    return true;
}

bool FbXmlHandler::NodeHandler::doEnd(const QString &name, bool & exists)
{
    if (m_handler) {
        bool found = exists || name == m_name;
        m_handler->doEnd(name, found);
        if (m_handler->m_closed) { delete m_handler; m_handler = NULL; }
        if (found) { exists = true; return true; }
    }
    bool found = name == m_name;
    m_closed = found || exists;
    if (m_closed) EndTag(m_name);
    exists = found;
    return true;
}

//---------------------------------------------------------------------------
//  FbXmlHandler
//---------------------------------------------------------------------------

FbXmlHandler::FbXmlHandler()
    : QXmlDefaultHandler()
    , m_handler(0)
{
}

FbXmlHandler::~FbXmlHandler()
{
    if (m_handler) delete m_handler;
}

bool FbXmlHandler::startElement(const QString & namespaceURI, const QString & localName, const QString &qName, const QXmlAttributes &attributes)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);
    const QString name = qName.toLower();
    if (m_handler) return m_handler->doStart(name, attributes);
    m_handler = CreateRoot(name, attributes);
    return m_handler;
}

bool FbXmlHandler::isWhiteSpace(const QString &str)
{
    return str.simplified().isEmpty();
}

bool FbXmlHandler::characters(const QString &str)
{
    QString s = str.simplified();
    if (s.isEmpty()) return true;
    if (isWhiteSpace(str.left(1))) s.prepend(" ");
    if (isWhiteSpace(str.right(1))) s.append(" ");
    return m_handler && m_handler->doText(s);
}

bool FbXmlHandler::endElement(const QString & namespaceURI, const QString & localName, const QString &qName)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);
    bool found = false;
    return m_handler && m_handler->doEnd(qName.toLower(), found);
}

bool FbXmlHandler::warning(const QXmlParseException& exception)
{
    emit warning(exception.lineNumber(), exception.columnNumber(), exception.message());
    return true;
}

bool FbXmlHandler::error(const QXmlParseException& exception)
{
    emit error(exception.lineNumber(), exception.columnNumber(), exception.message());
    return false;
}

bool FbXmlHandler::fatalError(const QXmlParseException &exception)
{
    emit fatal(exception.lineNumber(), exception.columnNumber(), exception.message());
    return false;
}

QString FbXmlHandler::errorString() const
{
    return m_error;
}
