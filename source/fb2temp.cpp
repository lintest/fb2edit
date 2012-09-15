#include "fb2temp.hpp"

#include <QAbstractListModel>
#include <QBuffer>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QImageReader>
#include <QUrl>
#include <QVBoxLayout>
#include <QtDebug>

#include "fb2text.hpp"

//---------------------------------------------------------------------------
//  FbTemporaryFile
//---------------------------------------------------------------------------

FbTemporaryFile::FbTemporaryFile(const QString &name)
    : QTemporaryFile()
    , m_name(name)
    , m_size(0)
{
}

qint64 FbTemporaryFile::write(QByteArray &data)
{
    open();
    if (m_hash.isEmpty()) m_hash = md5(data);
    m_size = QTemporaryFile::write(data);
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);
    m_type = QImageReader::imageFormat(&buffer);
    close();
    return m_size;
}

QString FbTemporaryFile::md5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toBase64();
}

QByteArray FbTemporaryFile::data()
{
    open();
    QByteArray data = readAll();
    close();
    return data;
}

//---------------------------------------------------------------------------
//  FbTemporaryList
//---------------------------------------------------------------------------

FbTemporaryList::FbTemporaryList()
{
}

FbTemporaryList::~FbTemporaryList()
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) delete it.next();
}

QString FbTemporaryList::add(const QString &path, QByteArray &data)
{
    QString hash = FbTemporaryFile::md5(data);
    QString name = this->name(hash);
    if (name.isEmpty()) {
        name = newName(path);
        FbTemporaryFile * temp = new FbTemporaryFile(name);
        temp->setHash(hash);
        temp->write(data);
        append(temp);
    }
    return name;
}

QString FbTemporaryList::newName(const QString &path)
{
    QFileInfo info(path);
    QString name = info.fileName();
    if (!exists(name)) return name;
    QString base = info.baseName();
    QString suff = info.suffix();
    for (int i = 1; ; i++) {
        QString name = QString("%1(%2).%3").arg(base).arg(i).arg(suff);
        if (exists(name)) continue;
        return name;
    }
}

FbTemporaryFile * FbTemporaryList::get(const QString &name) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbTemporaryFile * file = it.next();
        if (file->name() == name) return file;
    }
    return NULL;
}

QByteArray FbTemporaryList::data(const QString &name) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbTemporaryFile *file = it.next();
        if (file->name() == name) return file->data();
    }
    return QByteArray();
}

const QString & FbTemporaryList::set(const QString &name, QByteArray &data, const QString &hash)
{
    FbTemporaryFile * file = get(name);
    if (!file) append(file = new FbTemporaryFile(name));
    file->setHash(hash);
    file->write(data);
    return file->hash();
}

QString FbTemporaryList::name(const QString &hash) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbTemporaryFile *file = it.next();
        if (file->hash() == hash) return file->name();
    }
    return QString();
}

bool FbTemporaryList::exists(const QString &name) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbTemporaryFile *file = it.next();
        if (file->name() == name) return true;
    }
    return false;
}

#if 0

//---------------------------------------------------------------------------
//  FbNetworkDiskCache
//---------------------------------------------------------------------------

QNetworkCacheMetaData FbNetworkDiskCache::metaData(const QUrl &url)
{
    qCritical() << url.toString();
    return QNetworkDiskCache::metaData(url);
}

QIODevice * FbNetworkDiskCache::data(const QUrl &url)
{
    qCritical() << url.toString();
    return QNetworkDiskCache::data(url);
}

#endif

//---------------------------------------------------------------------------
//  FbImageReply
//---------------------------------------------------------------------------

FbImageReply::FbImageReply(QNetworkAccessManager::Operation op, const QNetworkRequest &request, const QByteArray &data)
    : QNetworkReply()
    , content(data)
    , offset(0)
{
    setOperation(op);
    setRequest(request);
    setUrl(request.url());
    open(ReadOnly | Unbuffered);
    setHeader(QNetworkRequest::ContentLengthHeader, QVariant(content.size()));
    setAttribute(QNetworkRequest::CacheSaveControlAttribute, QVariant(false));
    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}

qint64 FbImageReply::readData(char *data, qint64 maxSize)
{
    if (offset >= content.size()) return -1;
    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    qint64 number = qMin(maxSize, content.size() - offset);
    memcpy(data, content.constData() + offset, number);
    offset += number;
    return number;
}

//---------------------------------------------------------------------------
//  FbNetworkAccessManager
//
//    http://doc.trolltech.com/qq/32/qq32-webkit-protocols.html
//---------------------------------------------------------------------------

FbNetworkAccessManager::FbNetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
//    QWebSettings::clearMemoryCaches();
}

QNetworkReply * FbNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    const QUrl &url = request.url();
    const QString path = url.path();
    if (url.scheme() == "fb2" && path == m_path) return imageRequest(op, request);
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

QNetworkReply * FbNetworkAccessManager::imageRequest(Operation op, const QNetworkRequest &request)
{
    QString name = request.url().fragment();
    QByteArray data = m_files.data(name);
    return new FbImageReply(op, request, data);
}

QVariant FbNetworkAccessManager::info(int row, int col) const
{
    if (0 <= row && row < count()) {
        FbTemporaryFile *file = m_files[row];
        switch (col) {
            case 0: return file->name();
            case 1: return file->type();
            case 2: return file->size();
        }
        return m_files[row]->name();
    }
    return QVariant();
}

QByteArray FbNetworkAccessManager::data(int index) const
{
    if (0 <= index && index < count()) {
        return m_files[index]->data();
    }
    return QByteArray();
}

void FbNetworkAccessManager::data(QString name, QByteArray data)
{
    m_files.set(name, data);
}

//---------------------------------------------------------------------------
//  FbListModel
//---------------------------------------------------------------------------

FbListModel::FbListModel(FbNetworkAccessManager &files, QObject *parent)
    : QAbstractListModel(parent)
    , m_files(files)
{
}

int FbListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

int FbListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_files.count();
}

QVariant FbListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("File name");
            case 1: return tr("Type");
            case 2: return tr("Size");
        }
    }
    return QVariant();
}

QVariant FbListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        switch (role) {
            case Qt::DisplayRole: {
                return m_files.info(index.row(), index.column());
            } break;
            case Qt::TextAlignmentRole: {
                switch (index.column()) {
                    case 2: return Qt::AlignRight;
                    default: return Qt::AlignLeft;
                }
            }
        }
    }
    return QVariant();
}

//---------------------------------------------------------------------------
//  FbListView
//---------------------------------------------------------------------------

#include <QSplitter>
#include <QScrollArea>

FbListView::FbListView(FbNetworkAccessManager *files, QWidget *parent)
    : QTreeView(parent)
    , m_files(*files)
{
    m_label = new QLabel(this);
    m_label->setScaledContents(true);
}

void FbListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTreeView::currentChanged(current, previous);

    int row = current.row();
    if (0 <= row && row < model()->temp().count()) {
        QPixmap pixmap;
        pixmap.loadFromData(model()->temp().data(row));
        m_label->setPixmap(pixmap);
        m_label->resize(pixmap.size());
    }
}

FbListModel * FbListView::model() const
{
    return qobject_cast<FbListModel*>(QTreeView::model());
}

//---------------------------------------------------------------------------
//  FbListWidget
//---------------------------------------------------------------------------

FbListWidget::FbListWidget(FbTextEdit *view, QWidget* parent)
    : QWidget(parent)
    , m_view(*view)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);

    m_list = new FbListView(view->files(), splitter);
    splitter->addWidget(m_list);

    QScrollArea *scroll = new QScrollArea(splitter);
    scroll->setWidget(m_list->label());
    splitter->addWidget(scroll);

    splitter->setSizes(QList<int>() << 1 << 1);
    layout->addWidget(splitter);

    connect(&m_view, SIGNAL(loadFinished(bool)), SLOT(loadFinished(bool)));
    loadFinished(true);

//    m_tool = new QToolBar(this);
//    layout->addWidget(m_tool);
}

void FbListWidget::loadFinished(bool ok)
{
    m_list->setModel(new FbListModel(*m_view.files(), this));
    m_list->label()->clear();
    m_list->reset();
    m_list->resizeColumnToContents(1);
    m_list->resizeColumnToContents(2);
}
