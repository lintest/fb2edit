#include <QtGui>
#include <QtDebug>

#include "fb2save.hpp"
#include "fb2view.hpp"
#include "fb2utils.h"

#include <QAbstractNetworkCache>
#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopedPointer>
#include <QTextCodec>
#include <QWebFrame>
#include <QWebPage>

//---------------------------------------------------------------------------
//  Fb2SaveDialog
//---------------------------------------------------------------------------

Fb2SaveDialog::Fb2SaveDialog(QWidget *parent, Qt::WindowFlags f)
    : QFileDialog(parent, f)
{
    init();
}

Fb2SaveDialog::Fb2SaveDialog(QWidget *parent, const QString &caption, const QString &directory, const QString &filter)
    : QFileDialog(parent, caption, directory, filter)
{
    init();
}

void Fb2SaveDialog::init()
{
    QMap<QString, QString> codecMap;
    foreach (int mib, QTextCodec::availableMibs()) {
        QTextCodec *codec = QTextCodec::codecForMib(mib);

        QString sortKey = codec->name().toUpper();
        int rank;

        if (sortKey.startsWith("UTF-8")) {
            rank = 1;
        } else if (sortKey.startsWith("KOI8")) {
            rank = 2;
        } else if (sortKey.startsWith("WINDOWS")) {
            rank = 3;
        } else {
            rank = 4;
        }
        sortKey.prepend(QChar('0' + rank));
        codecMap.insert(sortKey, codec->name());
    }

    setAcceptMode(AcceptSave);
    setConfirmOverwrite(true);
    setDefaultSuffix("fb2");

    QStringList filters;
    filters << tr("Fiction book files (*.fb2)");
    filters << tr("Any files (*.*)");
    setNameFilters(filters);

    combo = new QComboBox(this);
    foreach (QString codec, codecMap) {
        combo->addItem(codec);
    }
    combo->setCurrentIndex(0);

    label = new QLabel(this);
    label->setText(tr("&Encoding"));
    label->setBuddy(combo);

    layout()->addWidget(label);
    layout()->addWidget(combo);
}

QString Fb2SaveDialog::fileName() const
{
    foreach (QString filename, selectedFiles()) {
        return filename;
    }
    return QString();
}

QString Fb2SaveDialog::codec() const
{
    return combo->currentText();
}

//---------------------------------------------------------------------------
//  Fb2HtmlHandler
//---------------------------------------------------------------------------

QString Fb2HtmlHandler::local(const QString &name)
{
    return name.mid(name.lastIndexOf(":"));
}

void Fb2HtmlHandler::onAttr(const QString &name, const QString &value)
{
    m_atts.append(name, "", local(name), value);
}

void Fb2HtmlHandler::onNew(const QString &name)
{
    startElement("", local(name), name, m_atts);
    m_atts.clear();
}

void Fb2HtmlHandler::onTxt(const QString &text)
{
    characters(text);
}

void Fb2HtmlHandler::onCom(const QString &text)
{
    comment(text);
}

void Fb2HtmlHandler::onEnd(const QString &name)
{
    endElement("", local(name), name);
}

//---------------------------------------------------------------------------
//  Fb2SaveWriter
//---------------------------------------------------------------------------

Fb2SaveWriter::Fb2SaveWriter(Fb2WebView &view, QByteArray *array)
    : QXmlStreamWriter(array)
    , m_view(view)
{
}

Fb2SaveWriter::Fb2SaveWriter(Fb2WebView &view, QIODevice *device)
    : QXmlStreamWriter(device)
    , m_view(view)
{
}

Fb2SaveWriter::Fb2SaveWriter(Fb2WebView &view, QString *string)
    : QXmlStreamWriter(string)
    , m_view(view)
{
}

void Fb2SaveWriter::writeComment(const QString &ch)
{
    writeLineEnd();
    QXmlStreamWriter::writeComment(ch);

}

void Fb2SaveWriter::writeStartElement(const QString &name, int level)
{
    if (level) writeLineEnd();
    for (int i = 1; i < level; i++) writeCharacters("  ");
    QXmlStreamWriter::writeStartElement(name);
}

void Fb2SaveWriter::writeEndElement(int level)
{
    if (level) writeLineEnd();
    for (int i = 1; i < level; i++) writeCharacters("  ");
    QXmlStreamWriter::writeEndElement();
}

void Fb2SaveWriter::writeLineEnd()
{
    writeCharacters("\n");
}

QByteArray Fb2SaveWriter::downloadFile(const QUrl &url)
{
    QNetworkRequest request(url);
    QNetworkAccessManager * network = m_view.page()->networkAccessManager();
    QScopedPointer<QNetworkReply> reply(network->get(request));
    if (reply.isNull()) return QByteArray();

    QEventLoop loop;
    QObject::connect(reply.data(), SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    return reply->readAll();
}

QString Fb2SaveWriter::getFileName(const QString &path)
{
    QUrl url = path;
    if (url.scheme() == "fb2") {
        QString name = url.path();
        if (m_view.files().exists(name)) {
            m_names.append(name);
            return name;
        }
        return QString();
    } else {
        QByteArray data = downloadFile(url);
        if (data.size() == 0) return QString();
        QString name = m_view.files().add(url.path(), data);
        m_names.append(name);
        return name;
    }
}

void Fb2SaveWriter::writeFiles()
{
    QStringListIterator it(m_names);
    while (it.hasNext()) {
        QString name = it.next();
        if (name.isEmpty()) continue;
        Fb2TemporaryFile * file = m_view.files().get(name);
        if (!file) continue;
        QString type = file->type();
        QString data = file->data().toBase64();
        writeStartElement("binary", 2);
        writeAttribute("id", name);
        if (!type.isEmpty()) writeAttribute("content-type", type);
        writeLineEnd();
        int pos = 0;
        while (true) {
            QString text = data.mid(pos, 76);
            if (text.isEmpty()) break;
            writeCharacters(text);
            writeLineEnd();
            pos += 76;
        }
        writeCharacters("  ");
        QXmlStreamWriter::writeEndElement();
    }
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::TextHandler
//---------------------------------------------------------------------------

FB2_BEGIN_KEYHASH(Fb2SaveHandler::TextHandler)
    FB2_KEY( Section , "div"    );
    FB2_KEY( Anchor  , "a"      );
    FB2_KEY( Image   , "img"  );
    FB2_KEY( Table   , "table"  );
    FB2_KEY( Parag   , "p"      );
    FB2_KEY( Strong  , "b"      );
    FB2_KEY( Emphas  , "i"      );
    FB2_KEY( Strike  , "strike" );
    FB2_KEY( Sub     , "sub"    );
    FB2_KEY( Sup     , "sup"    );
    FB2_KEY( Code    , "tt"     );
FB2_END_KEYHASH

Fb2SaveHandler::TextHandler::TextHandler(Fb2SaveWriter &writer, const QString &name, const QXmlAttributes &atts, const QString &tag)
    : NodeHandler(name)
    , m_writer(writer)
    , m_tag(tag)
    , m_level(1)
    , m_hasChild(false)
{
    Init(atts);
}

Fb2SaveHandler::TextHandler::TextHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts, const QString &tag)
    : NodeHandler(name)
    , m_writer(parent->m_writer)
    , m_tag(tag)
    , m_level(parent->nextLevel())
    , m_hasChild(false)
{
    Init(atts);
}

void Fb2SaveHandler::TextHandler::Init(const QXmlAttributes &atts)
{
    if (m_tag.isEmpty()) return;
    m_writer.writeStartElement(m_tag, m_level);
    int count = atts.count();
    for (int i = 0; i < count; i++) {
        QString name = atts.qName(i);
        if (name == "id") {
            m_writer.writeAttribute(name, atts.value(i));
        } else if (name == "name") {
            m_writer.writeAttribute(name, atts.value(i));
        } else if (name.left(4) == "fb2:") {
            m_writer.writeAttribute(name.mid(4), atts.value(i));
        }
    }
}

Fb2XmlHandler::NodeHandler * Fb2SaveHandler::TextHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    m_hasChild = true;
    QString tag = QString();
    switch (toKeyword(name)) {
        case Section   : tag = atts.value("class") ; break;
        case Anchor    : return new AnchorHandler(this, name, atts);
        case Image     : return new ImageHandler(this, name, atts);
        case Parag     : return new ParagHandler(this, name, atts);
        case Strong    : tag = "strong"        ; break;
        case Emphas    : tag = "emphasis"      ; break;
        case Strike    : tag = "strikethrough" ; break;
        case Code      : tag = "code"          ; break;
        case Sub       : tag = "sub"           ; break;
        case Sup       : tag = "sup"           ; break;
        default: ;
    }
    return new TextHandler(this, name, atts, tag);
}

void Fb2SaveHandler::TextHandler::TxtTag(const QString &text)
{
    m_writer.writeCharacters(text);
}

void Fb2SaveHandler::TextHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_tag.isEmpty()) return;
    m_writer.writeEndElement(m_hasChild ? m_level : 0);
}

int Fb2SaveHandler::TextHandler::nextLevel() const
{
    return m_level ? m_level + 1 : 0;
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::RootHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::RootHandler::RootHandler(Fb2SaveWriter &writer, const QString &name)
    : NodeHandler(name)
    , m_writer(writer)
{
}

Fb2XmlHandler::NodeHandler * Fb2SaveHandler::RootHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    return name == "body" ? new BodyHandler(m_writer, name, atts) : NULL;
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::BodyHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::BodyHandler::BodyHandler(Fb2SaveWriter &writer, const QString &name, const QXmlAttributes &atts)
    : TextHandler(writer, name, atts, "FictionBook")
{
    m_writer.writeAttribute("xmlns", "http://www.gribuser.ru/xml/fictionbook/2.0");
    m_writer.writeAttribute("xmlns:l", "http://www.w3.org/1999/xlink");
}

void Fb2SaveHandler::BodyHandler::EndTag(const QString &name)
{
    m_writer.writeFiles();
    TextHandler::EndTag(name);
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::AnchorHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::AnchorHandler::AnchorHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts)
    : TextHandler(parent, name, atts, "a")
{
    QString href = Value(atts, "href");
    m_writer.writeAttribute("l:href", href);
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::ImageHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::ImageHandler::ImageHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts)
    : TextHandler(parent, name, atts, "image")
{
    QString src = Value(atts, "src");
    QString file = m_writer.getFileName(src);
    file.prepend('#');
    m_writer.writeAttribute("l:href", file);
    m_writer.writeEndElement(0);
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler::ParagHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::ParagHandler::ParagHandler(TextHandler *parent, const QString &name, const QXmlAttributes &atts)
    : TextHandler(parent, name, atts, "")
    , m_parent(parent->tag())
    , m_empty(true)
{
}

Fb2XmlHandler::NodeHandler * Fb2SaveHandler::ParagHandler::NewTag(const QString &name, const QXmlAttributes &atts)
{
    if (m_empty) start();
    return TextHandler::NewTag(name, atts);
}

void Fb2SaveHandler::ParagHandler::TxtTag(const QString &text)
{
    if (m_empty) {
        if (isWhiteSpace(text)) return;
        start();
    }
    TextHandler::TxtTag(text);
}

void Fb2SaveHandler::ParagHandler::EndTag(const QString &name)
{
    Q_UNUSED(name);
    if (m_empty) m_writer.writeStartElement("empty-line", m_level);
    m_writer.writeEndElement(0);
}

void Fb2SaveHandler::ParagHandler::start()
{
    if (!m_empty) return;
    QString tag = m_parent == "stanza" ? "v" : "p";
    m_writer.writeStartElement(tag, m_level);
    m_empty = false;
}

//---------------------------------------------------------------------------
//  Fb2SaveHandler
//---------------------------------------------------------------------------

Fb2SaveHandler::Fb2SaveHandler(Fb2SaveWriter &writer)
    : Fb2HtmlHandler()
    , m_writer(writer)
{
}

bool Fb2SaveHandler::comment(const QString& ch)
{
    m_writer.writeComment(ch);
    return true;
}

Fb2XmlHandler::NodeHandler * Fb2SaveHandler::CreateRoot(const QString &name, const QXmlAttributes &atts)
{
    Q_UNUSED(atts);
    if (name == "html") return new RootHandler(m_writer, name);
    m_error = QObject::tr("The tag <html> was not found.");
    return 0;
}

bool Fb2SaveHandler::save()
{
    m_writer.writeStartDocument();
    QWebFrame * frame = m_writer.view().page()->mainFrame();
    static const QString javascript = FB2::read(":/js/export.js");
    frame->addToJavaScriptWindowObject("handler", this);
    frame->evaluateJavaScript(javascript);
    m_writer.writeEndDocument();
    return true;
}
