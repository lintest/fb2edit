#include "fb2temp.hpp"

#include <QAbstractListModel>
#include <QBuffer>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QImageReader>
#include <QUrl>
#include <QWebFrame>
#include <QVBoxLayout>
#include <QtDebug>

#include "fb2page.hpp"
#include "fb2text.hpp"

//---------------------------------------------------------------------------
//  FbBinary
//---------------------------------------------------------------------------

FbBinary::FbBinary(const QString &name)
    : QTemporaryFile()
    , m_name(name)
    , m_size(0)
{
}

qint64 FbBinary::write(QByteArray &data)
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

QString FbBinary::md5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toBase64();
}

QByteArray FbBinary::data()
{
    open();
    QByteArray data = readAll();
    close();
    return data;
}

//---------------------------------------------------------------------------
//  FbStore
//---------------------------------------------------------------------------

FbStore::FbStore(QObject *parent)
    : QObject(parent)
{
}

FbStore::~FbStore()
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) delete it.next();
}

void FbStore::binary(const QString &name, const QByteArray &data)
{
    set(name, data);
}

QString FbStore::add(const QString &path, QByteArray &data)
{
    QString hash = FbBinary::md5(data);
    QString name = this->name(hash);
    if (name.isEmpty()) {
        name = newName(path);
        FbBinary * temp = new FbBinary(name);
        temp->setHash(hash);
        temp->write(data);
        append(temp);
    }
    return name;
}

QString FbStore::newName(const QString &path)
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

FbBinary * FbStore::get(const QString &name) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbBinary * file = it.next();
        if (file->name() == name) return file;
    }
    return NULL;
}

QByteArray FbStore::data(const QString &name) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbBinary *file = it.next();
        if (file->name() == name) return file->data();
    }
    return QByteArray();
}

const QString & FbStore::set(const QString &name, QByteArray data, const QString &hash)
{
    FbBinary * file = get(name);
    if (!file) append(file = new FbBinary(name));
    file->setHash(hash);
    file->write(data);
    return file->hash();
}

QString FbStore::name(const QString &hash) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbBinary *file = it.next();
        if (file->hash() == hash) return file->name();
    }
    return QString();
}

bool FbStore::exists(const QString &name) const
{
    FbTemporaryIterator it(*this);
    while (it.hasNext()) {
        FbBinary *file = it.next();
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
    , m_store(new FbStore(this))
{
}

void FbNetworkAccessManager::setStore(const QUrl url, FbStore *store)
{
    m_path = url.path();
    if (m_store) delete m_store;
    if (!store) store = new FbStore(this);
    store->setParent(this);
    m_store = store;
}

QNetworkReply * FbNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    if (m_store) {
        const QUrl &url = request.url();
        const QString path = url.path();
        if (url.scheme() == "fb2" && path == m_path) {
            QString name = request.url().fragment();
            QByteArray data = m_store->data(name);
            return new FbImageReply(op, request, data);
        }
    }
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

QVariant FbNetworkAccessManager::info(int row, int col) const
{
    if (!m_store) return QVariant();
    if (0 <= row && row < count()) {
        FbBinary *file = m_store->at(row);
        switch (col) {
            case 2: return file->type();
            case 3: return file->size();
        }
        return m_store->at(row)->name();
    }
    return QVariant();
}

QByteArray FbNetworkAccessManager::data(int index) const
{
    if (!m_store) return QByteArray();
    if (0 <= index && index < count()) {
        return m_store->at(index)->data();
    }
    return QByteArray();
}

//---------------------------------------------------------------------------
//  FbListModel
//---------------------------------------------------------------------------

FbListModel::FbListModel(FbTextEdit *text, QObject *parent)
    : QAbstractListModel(parent)
    , m_text(text)
{
}

int FbListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
}

int FbListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    FbNetworkAccessManager * f = files();
    return f ? f->count() : 0;
}

QVariant FbListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 1: return tr("File name");
            case 2: return tr("Type");
            case 3: return tr("Size");
        }
    }
    return QVariant();
}

FbNetworkAccessManager * FbListModel::files() const
{
    if (FbTextPage *page = qobject_cast<FbTextPage*>(m_text->page())) {
        return qobject_cast<FbNetworkAccessManager*>(page->networkAccessManager());
    } else {
        return 0;
    }
}

QVariant FbListModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        switch (role) {
            case Qt::DisplayRole: {
                FbNetworkAccessManager * f = files();
                return f ? f->info(index.row(), index.column()) : QVariant();
            } break;
            case Qt::TextAlignmentRole: {
                switch (index.column()) {
                    case 3: return Qt::AlignRight;
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

FbListView::FbListView(QWidget *parent)
    : QTreeView(parent)
{
    setAllColumnsShowFocus(true);
}

void FbListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTreeView::currentChanged(current, previous);
    QModelIndex index = model()->index(current.row(), 0);
    emit showImage(model()->data(index).toString());
}

FbListModel * FbListView::model() const
{
    return qobject_cast<FbListModel*>(QTreeView::model());
}

//---------------------------------------------------------------------------
//  FbListWidget
//---------------------------------------------------------------------------

FbListWidget::FbListWidget(FbTextEdit *text, QWidget* parent)
    : QWidget(parent)
    , m_text(text)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);

    m_list = new FbListView(splitter);
    splitter->addWidget(m_list);

    FbTextFrame *frame = new FbTextFrame(splitter);
    splitter->addWidget(frame);

    m_view = new FbTextBase(frame);
    m_view->page()->setNetworkAccessManager(text->page()->networkAccessManager());
    frame->layout()->addWidget(m_view);

    splitter->setSizes(QList<int>() << 100 << 100);

    layout->addWidget(splitter);

    connect(m_text, SIGNAL(loadFinished(bool)), SLOT(loadFinished()));
    connect(m_list, SIGNAL(showImage(QString)), SLOT(showImage(QString)));
    loadFinished();
}

void FbListWidget::loadFinished()
{
    m_view->load(QUrl());

    m_list->setModel(new FbListModel(m_text, this));
    m_list->reset();
    m_list->resizeColumnToContents(1);
    m_list->resizeColumnToContents(2);
    m_list->resizeColumnToContents(3);
    m_list->setColumnHidden(0, true);
}

void FbListWidget::showImage(const QString &name)
{
    QUrl url = m_text->page()->mainFrame()->url();
    url.setFragment(name);
    QString html = QString("<img src=%1 valign=center align=center width=100%>").arg(url.toString());
    m_view->setHtml(html);
}
