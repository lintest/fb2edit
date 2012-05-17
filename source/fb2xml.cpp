#include "fb2xml.h"
#include <QtDebug>

//---------------------------------------------------------------------------
//  Fb2XmlHandler::NodeHandler
//---------------------------------------------------------------------------

QString Fb2XmlHandler::NodeHandler::Value(const QXmlAttributes &attributes, const QString &name)
{
    int count = attributes.count();
    for (int i = 0; i < count; i++ ) {
        if (attributes.localName(i).compare(name, Qt::CaseInsensitive) == 0) {
            return attributes.value(i);
        }
    }
    return QString();
}

bool Fb2XmlHandler::NodeHandler::doStart(const QString &name, const QXmlAttributes &attributes)
{
    if (m_handler) return m_handler->doStart(name, attributes);
    m_handler = NewTag(name, attributes); if (m_handler) return true;
    m_handler = new NodeHandler(name);
    return true;
}

bool Fb2XmlHandler::NodeHandler::doText(const QString &text)
{
    if (m_handler) m_handler->doText(text); else TxtTag(text);
    return true;
}

bool Fb2XmlHandler::NodeHandler::doEnd(const QString &name, bool & exists)
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
//  Fb2XmlHandler
//---------------------------------------------------------------------------

Fb2XmlHandler::Fb2XmlHandler()
    : QXmlDefaultHandler()
    , m_handler(0)
{
}

Fb2XmlHandler::~Fb2XmlHandler()
{
    if (m_handler) delete m_handler;
}

bool Fb2XmlHandler::startElement(const QString & namespaceURI, const QString & localName, const QString &qName, const QXmlAttributes &attributes)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);
    const QString name = qName.toLower();
    if (m_handler) return m_handler->doStart(name, attributes);
    return m_handler = CreateRoot(name, attributes);
}

bool Fb2XmlHandler::isWhiteSpace(const QString &str)
{
    return str.simplified().isEmpty();
}

bool Fb2XmlHandler::characters(const QString &str)
{
    QString s = str.simplified();
    if (s.isEmpty()) return true;
    if (isWhiteSpace(str.left(1))) s.prepend(" ");
    if (isWhiteSpace(str.right(1))) s.append(" ");
    return m_handler && m_handler->doText(s);
}

bool Fb2XmlHandler::endElement(const QString & namespaceURI, const QString & localName, const QString &qName)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);
    bool found = false;
    return m_handler && m_handler->doEnd(qName.toLower(), found);
}

bool Fb2XmlHandler::error(const QXmlParseException& exception)
{
    qCritical() << QObject::tr("Parse error at line %1, column %2: %3")
       .arg(exception.lineNumber())
       .arg(exception.columnNumber())
       .arg(exception.message().simplified());
    return false;
}

bool Fb2XmlHandler::warning(const QXmlParseException& exception)
{
    qWarning() << QObject::tr("Parse error at line %1, column %2: %3")
       .arg(exception.lineNumber())
       .arg(exception.columnNumber())
       .arg(exception.message().simplified());
    return false;
}

bool Fb2XmlHandler::fatalError(const QXmlParseException &exception)
{
    qCritical() << QObject::tr("Parse error at line %1, column %2: %3")
       .arg(exception.lineNumber())
       .arg(exception.columnNumber())
       .arg(exception.message().simplified());
    return false;
}

QString Fb2XmlHandler::errorString() const
{
    return m_error;
}

