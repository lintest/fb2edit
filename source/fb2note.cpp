#include "fb2note.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSplitter>
#include <QToolBar>
#include <QWebFrame>
#include <QWebPage>
#include <QWebView>

#include "fb2list.hpp"
#include "fb2page.hpp"
#include "fb2text.hpp"
#include "fb2html.h"

//---------------------------------------------------------------------------
//  FbNoteDlg
//---------------------------------------------------------------------------

FbNoteDlg::FbNoteDlg(FbTextBase *text)
    : QDialog(text)
{
    setWindowTitle(tr("Insert footnote"));
    resize(460, 300);

    QGridLayout * gridLayout = new QGridLayout(this);

    QLabel * label1 = new QLabel(this);
    label1->setText(tr("Identifier:"));
    gridLayout->addWidget(label1, 0, 0, 1, 1);

    m_key = new QComboBox(this);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    gridLayout->addWidget(m_key, 0, 1, 1, 1);

    QLabel * label2 = new QLabel(this);
    label2->setText(tr("Title:"));
    gridLayout->addWidget(label2, 1, 0, 1, 1);

    m_title = new QLineEdit(this);
    m_title->setObjectName(QString::fromUtf8("m_title"));
    gridLayout->addWidget(m_title, 1, 1, 1, 1);

    m_toolbar = new QToolBar(this);
    gridLayout->addWidget(m_toolbar, 2, 0, 1, 2);

    QFrame * frame = new QFrame(this);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Sunken);
    gridLayout->addWidget(frame, 3, 0, 1, 2);

    QLayout * frameLayout = new QHBoxLayout(frame);
    frameLayout->setSpacing(0);
    frameLayout->setMargin(0);

    m_text = new FbTextBase(frame);
    m_text->setObjectName(QString::fromUtf8("m_text"));
    m_text->setUrl(QUrl(QString::fromUtf8("about:blank")));
    frameLayout->addWidget(m_text);

    QDialogButtonBox * buttonBox = new QDialogButtonBox(this);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    gridLayout->addWidget(buttonBox, 4, 0, 1, 2);

    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_key->addItem(tr("<create new>"));
    m_key->setCurrentIndex(0);
    m_title->setFocus();

    FbTextPage *page = new FbTextPage(this);
    connect(m_text, SIGNAL(loadFinished(bool)), SLOT(loadFinished()));
    page->setNetworkAccessManager(text->page()->networkAccessManager());
    page->setContentEditable(true);
    m_text->setPage(page);
    m_text->setHtml("<body><p><br></p></body>");

    m_text->addTools(m_toolbar);
}

void FbNoteDlg::loadFinished()
{
    FbTextElement body = m_text->page()->mainFrame()->documentElement().findFirst("body");
    body.select();
}

//---------------------------------------------------------------------------
//  FbNotesModel
//---------------------------------------------------------------------------

FbNotesModel::FbNotesModel(FbTextPage *page, QObject *parent)
    : QAbstractListModel(parent)
    , collection(page->body().findAll("a"))
    , m_page(page)
{
}

int FbNotesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
}

int FbNotesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : collection.count();
}

QVariant FbNotesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 1: return tr("Title");
            case 2: return tr("Type");
            case 3: return tr("Link");
        }
    }
    return QVariant();
}

QVariant FbNotesModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        switch (role) {
            case Qt::DisplayRole: {
                int row = index.row();
                if (row < 0 || row >= collection.count()) return QVariant();
                QWebElement element = collection[row];
                switch (index.column()) {
                    case 1: return element.toPlainText();
                    case 2: return element.attribute("type");
                    default: return element.attribute("href");
                }
            } break;
            case Qt::TextAlignmentRole: {
                return Qt::AlignLeft;
            }
        }
    }
    return QVariant();
}

//---------------------------------------------------------------------------
//  FbNotesWidget
//---------------------------------------------------------------------------

FbNotesWidget::FbNotesWidget(FbTextEdit *text, QWidget* parent)
    : QWidget(parent)
    , m_text(text)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);

    m_list = new FbListView(splitter);
    m_list->header()->setDefaultSectionSize(50);
    splitter->addWidget(m_list);

    FbTextFrame *frame = new FbTextFrame(splitter);
    splitter->addWidget(frame);

    m_view = new FbTextBase(frame);
    m_view->page()->setNetworkAccessManager(text->page()->networkAccessManager());
    m_view->page()->settings()->setUserStyleSheetUrl(QUrl::fromLocalFile(":style.css"));
    frame->layout()->addWidget(m_view);

    splitter->setSizes(QList<int>() << 100 << 100);

    layout->addWidget(splitter);

    connect(m_text, SIGNAL(loadFinished(bool)), SLOT(loadFinished()));
    connect(m_list, SIGNAL(showCurrent(QString)), SLOT(showCurrent(QString)));
    loadFinished();
}

void FbNotesWidget::loadFinished()
{
    if (QAbstractItemModel *m = m_list->model()) m->deleteLater();
    m_view->load(QUrl());
    m_list->setModel(new FbNotesModel(m_text->page(), this));
    m_list->reset();
    m_list->setColumnHidden(0, true);
}

void FbNotesWidget::showCurrent(const QString &name)
{
    QWebElement element = m_text->body().findFirst(name);
    QString html = element.toInnerXml();
    html.prepend(
        "<fb:body name=notes style='padding:0;margin:0;'>"
        "<fb:section id=0 style='border:0;padding:0;margin:0;'>"
    );
    html.append("</fb:section></fb:body>");
    m_view->setHtml(html, m_view->url());
}
