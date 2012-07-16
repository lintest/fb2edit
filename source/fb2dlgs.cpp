#include "fb2dlgs.hpp"
#include "fb2code.hpp"
#include "fb2tree.hpp"
#include "fb2view.hpp"
#include "fb2utils.h"
#include "ui_fb2find.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QToolBar>
#include <QWebFrame>
#include <QWebPage>

//---------------------------------------------------------------------------
//  Fb2CodeFindDlg
//---------------------------------------------------------------------------

Fb2CodeFindDlg::Fb2CodeFindDlg(Fb2CodeEdit &edit)
    : QDialog(&edit)
    , ui(new Ui::Fb2Find)
    , m_edit(edit)
{
    ui->setupUi(this);
    ui->checkHigh->setText(tr("Complete words"));
    connect(ui->btnFind, SIGNAL(clicked()), this, SLOT(find()));
}

Fb2CodeFindDlg::~Fb2CodeFindDlg()
{
    delete ui;
}

void Fb2CodeFindDlg::find()
{
    QString text = ui->editText->text();
    if (text.isEmpty()) return;
    QTextDocument::FindFlags options = 0;
    if (ui->radioUp->isChecked()) options |= QTextDocument::FindBackward;
    if (ui->checkCase->isChecked()) options |= QTextDocument::FindCaseSensitively;
    if (ui->checkHigh->isChecked()) options |= QTextDocument::FindWholeWords;

    m_edit.findText(text, options);
}

//---------------------------------------------------------------------------
//  Fb2TextFindDlg
//---------------------------------------------------------------------------

Fb2TextFindDlg::Fb2TextFindDlg(Fb2TextEdit &edit)
    : QDialog(&edit)
    , ui(new Ui::Fb2Find)
    , m_edit(edit)
{
    ui->setupUi(this);
    connect(ui->btnFind, SIGNAL(clicked()), this, SLOT(find()));
}

Fb2TextFindDlg::~Fb2TextFindDlg()
{
    m_edit.findText(QString(), QWebPage::HighlightAllOccurrences);
    delete ui;
}

void Fb2TextFindDlg::find()
{
    QString text = ui->editText->text();
    if (text.isEmpty()) return;
    QWebPage::FindFlags options = QWebPage::FindWrapsAroundDocument;
    if (ui->radioUp->isChecked()) options |= QWebPage::FindBackward;
    if (ui->checkCase->isChecked()) options |= QWebPage::FindCaseSensitively;
    if (ui->checkHigh->isChecked()) options |= QWebPage::HighlightAllOccurrences;

    m_edit.findText(text, options);
}

//---------------------------------------------------------------------------
//  Fb2NoteDlg
//---------------------------------------------------------------------------

Fb2NoteDlg::Fb2NoteDlg(Fb2TextEdit &view)
    : QDialog(&view)
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

    QLayout * frameLayout = new QBoxLayout(QBoxLayout::LeftToRight, frame);
    frameLayout->setSpacing(0);
    frameLayout->setMargin(0);

    m_text = new QWebView(frame);
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

    Fb2TextPage *page = new Fb2TextPage(this);
    connect(m_text, SIGNAL(loadFinished(bool)), SLOT(loadFinished()));
    page->setNetworkAccessManager(view.page()->networkAccessManager());
    page->setContentEditable(true);
    m_text->setPage(page);
    m_text->setHtml("<body><p><br></p></body>");

    FB2::addTools(m_toolbar, m_text);
}

void Fb2NoteDlg::loadFinished()
{
    Fb2TextElement body = m_text->page()->mainFrame()->documentElement().findFirst("body");
    body.select();
}
