#include "fb2imgs.hpp"

#include <QAbstractListModel>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QSplitter>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebFrame>
#include <QTabWidget>
#include <QtDebug>

#include "fb2list.hpp"
#include "fb2page.hpp"
#include "fb2text.hpp"
#include "fb2utils.h"

static QString imgHtml(const QUrl &url)
{
    return QString("<img src=\"%1\" valign=center align=center width=100%>").arg(url.toString());
}

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
    for (int i = 1; ; ++i) {
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
//  FbComboCtrl
//---------------------------------------------------------------------------

FbComboCtrl::FbComboCtrl(QWidget *parent)
    : QLineEdit(parent)
{
    button = new QToolButton(this);
    button->setCursor(Qt::ArrowCursor);
    button->setFocusPolicy(Qt::NoFocus);
    connect(button, SIGNAL(clicked()), SIGNAL(popup()));
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(button, 0, Qt::AlignRight);
    layout->setSpacing(0);
    layout->setMargin(0);
}

void FbComboCtrl::resizeEvent(QResizeEvent* event)
{
    QLineEdit::resizeEvent(event);
    QMargins margins(0, 0, button->width(), 0);
    setTextMargins(margins);
}

void FbComboCtrl::setIcon(const QIcon &icon)
{
    button->setIcon(icon);
}

//---------------------------------------------------------------------------
//  FbImageDlg::FbTab
//---------------------------------------------------------------------------

FbImageDlg::FbTab::FbTab(QWidget* parent, QAbstractItemModel *model)
    : QWidget(parent)
    , combo(0)
    , edit(0)
{
    QGridLayout * layout = new QGridLayout(this);

    label = new QLabel(this);
    label->setText(tr("File name:"));
    layout->addWidget(label, 0, 0, 1, 1);

    QWidget *control;
    if (model) {
        control = combo = new QComboBox(this);
        combo->setModel(model);
    } else {
        control = edit = new FbComboCtrl(this);
    }

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    control->setSizePolicy(sizePolicy);
    layout->addWidget(control, 0, 1, 1, 1);

    QFrame *frame = new FbTextFrame(this);
    frame->setMinimumSize(QSize(300, 200));
    layout->addWidget(frame, 1, 0, 1, 2);

    preview = new QWebView(this);
    frame->layout()->addWidget(preview);
}

//---------------------------------------------------------------------------
//  FbImageDlg
//---------------------------------------------------------------------------

FbImageDlg::FbImageDlg(FbTextEdit *text)
    : QDialog(text)
    , owner(text)
    , tabFile(0)
    , tabPict(0)
{
    setWindowTitle(tr("Insert picture"));

    QLayout *layout = new QVBoxLayout(this);

    notebook = new QTabWidget(this);
    layout->addWidget(notebook);

    QDialogButtonBox *buttons = new QDialogButtonBox(this);
    buttons->setOrientation(Qt::Horizontal);
    buttons->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    layout->addWidget(buttons);

    connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    connect(notebook, SIGNAL(currentChanged(int)), SLOT(notebookChanged(int)));

    tabFile = new FbTab(notebook);
    tabFile->edit->setIcon(FbIcon("document-open"));
    tabFile->preview->setHtml(QString());
    connect(tabFile->edit, SIGNAL(textChanged(QString)), SLOT(filenameChanged(QString)));
    connect(tabFile->edit, SIGNAL(popup()), SLOT(selectFile()));
    notebook->addTab(tabFile, tr("Select file"));

    if (text->store()->count()) {
        FbImgsModel *model = new FbImgsModel(text, this);
        tabPict = new FbTab(notebook, model);
        tabPict->combo->setCurrentIndex(0);
        tabPict->preview->setHtml(QString(), text->url());
        tabPict->preview->page()->setNetworkAccessManager(text->page()->networkAccessManager());
        notebook->addTab(tabPict, tr("From collection"));
        connect(tabPict->combo, SIGNAL(activated(QString)), SLOT(pictureActivated(QString)));
    }

    tabFile->edit->setFocus();
    resize(minimumSizeHint());
}

void FbImageDlg::notebookChanged(int index)
{
    if (index) {
        disconnect(notebook, SIGNAL(currentChanged(int)), this, SLOT(notebookChanged(int)));
        if (tabPict) pictureActivated(tabPict->combo->itemText(0));
    }
}

void FbImageDlg::selectFile()
{
    QString filters;
    filters += tr("Common Graphics (*.png *.jpg *.jpeg *.gif)") += ";;";
    filters += tr("Portable Network Graphics (PNG) (*.png)") += ";;";
    filters += tr("JPEG (*.jpg *.jpeg)") += ";;";
    filters += tr("Graphics Interchange Format (*.gif)") += ";;";
    filters += tr("All Files (*)");
    QWidget *p = qobject_cast<QWidget*>(parent());
    QString path = QFileDialog::getOpenFileName(p, tr("Insert image..."), QString(), filters);
    if (path.isEmpty()) return;
    tabFile->edit->setText(path);
}

void FbImageDlg::filenameChanged(const QString & text)
{
    if (QFileInfo(text).exists()) {
        QUrl url = QUrl::fromLocalFile(text);
        preview(tabFile->preview, url);
    } else {
        tabFile->preview->setHtml(QString());
    }
}

void FbImageDlg::pictureActivated(const QString & text)
{
    QUrl url = tabPict->preview->url();
    url.setFragment(text);
    preview(tabPict->preview, url);
}

void FbImageDlg::preview(QWebView *preview, const QUrl &url)
{
    preview->setHtml(imgHtml(url), preview->url());
}

QString FbImageDlg::result() const
{
    if (tabPict && notebook->currentWidget() == tabPict) {
        return tabPict->combo->currentText();
    } else if (notebook->currentWidget() == tabFile) {
        QString path = tabFile->edit->text();
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QNetworkAccessManager *m = owner->page()->networkAccessManager();
            FbNetworkAccessManager *manager = qobject_cast<FbNetworkAccessManager*>(m);
            QByteArray data = file.readAll();
            return manager->add(path, data);
        }
    }
    return QString();
}

//---------------------------------------------------------------------------
//  FbImgsModel
//---------------------------------------------------------------------------

FbImgsModel::FbImgsModel(FbTextEdit *text, QObject *parent)
    : QAbstractListModel(parent)
{
    manager = qobject_cast<FbNetworkAccessManager*>(text->page()->networkAccessManager());
}

int FbImgsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
}

int FbImgsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : manager->count();
}

QVariant FbImgsModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant FbImgsModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        switch (role) {
            case Qt::DisplayRole: {
                return manager->info(index.row(), index.column());
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
//  FbImgsWidget
//---------------------------------------------------------------------------

FbImgsWidget::FbImgsWidget(FbTextEdit *text, QWidget* parent)
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
    connect(m_list, SIGNAL(showCurrent(QString)), SLOT(showCurrent(QString)));
    loadFinished();
}

void FbImgsWidget::loadFinished()
{
    if (QAbstractItemModel *m = m_list->model()) m->deleteLater();
    m_view->load(QUrl());
    m_list->setModel(new FbImgsModel(m_text, this));
    m_list->reset();
    m_list->resizeColumnToContents(1);
    m_list->resizeColumnToContents(2);
    m_list->resizeColumnToContents(3);
    m_list->setColumnHidden(0, true);
}

void FbImgsWidget::showCurrent(const QString &name)
{
    QUrl url = m_text->url();
    url.setFragment(name);
    m_view->setHtml(imgHtml(url));
}

