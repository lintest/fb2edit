#include "fb2dlgs.hpp"
#include "fb2code.hpp"
#include "fb2page.hpp"
#include "fb2text.hpp"
#include "fb2tree.hpp"
#include "fb2utils.h"
#include "ui_fb2find.h"
#include "ui_fb2setup.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QToolBar>
#include <QWebFrame>
#include <QWebPage>

//---------------------------------------------------------------------------
//  FbCodeFindDlg
//---------------------------------------------------------------------------

FbCodeFindDlg::FbCodeFindDlg(FbCodeEdit &edit)
    : QDialog(&edit)
    , ui(new Ui::FbFind)
    , m_edit(edit)
{
    ui->setupUi(this);
    ui->checkHigh->setText(tr("Complete words"));
    connect(ui->btnFind, SIGNAL(clicked()), this, SLOT(find()));
}

FbCodeFindDlg::~FbCodeFindDlg()
{
    delete ui;
}

void FbCodeFindDlg::find()
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
//  FbTextFindDlg
//---------------------------------------------------------------------------

FbTextFindDlg::FbTextFindDlg(FbTextEdit &edit)
    : QDialog(&edit)
    , ui(new Ui::FbFind)
    , m_edit(edit)
{
    ui->setupUi(this);
    ui->checkHigh->hide();
    connect(ui->btnFind, SIGNAL(clicked()), this, SLOT(find()));
}

FbTextFindDlg::~FbTextFindDlg()
{
    m_edit.findText(QString(), QWebPage::HighlightAllOccurrences);
    delete ui;
}

void FbTextFindDlg::find()
{
    QString text = ui->editText->text();
    if (text.isEmpty()) return;
    QWebPage::FindFlags options = QWebPage::FindWrapsAroundDocument;
    if (ui->radioUp->isChecked()) options |= QWebPage::FindBackward;
    if (ui->checkCase->isChecked()) options |= QWebPage::FindCaseSensitively;
    m_edit.findText(text, options);

    options |= QWebPage::HighlightAllOccurrences;
    m_edit.findText(text, options);
}

//---------------------------------------------------------------------------
//  FbNoteDlg
//---------------------------------------------------------------------------

FbNoteDlg::FbNoteDlg(FbTextEdit &view)
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
    page->setNetworkAccessManager(view.page()->networkAccessManager());
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
//  FbSetupDlg
//---------------------------------------------------------------------------

FbSetupDlg::FbSetupDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FbSetup)
{
    ui->setupUi(this);
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

    QUrl url = text->url();

    tabFile = new FbTab(notebook);
    tabFile->edit->setIcon(FbIcon("document-open"));
    tabFile->preview->setHtml(QString(), url);
    connect(tabFile->edit, SIGNAL(popup()), SLOT(selectFile()));
    notebook->addTab(tabFile, tr("Select file"));

    if (text->store()->count()) {
        FbListModel *model = new FbListModel(text, this);
        tabPict = new FbTab(notebook, model);
        tabPict->preview->setHtml(QString(), url);
        tabPict->combo->setCurrentIndex(0);
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
    if (!path.isEmpty()) tabFile->edit->setText(path);
}

void FbImageDlg::pictureActivated(const QString & text)
{
    QUrl url = tabPict->preview->url();
    url.setFragment(text);
    QString html = QString("<img src=%1 valign=center align=center width=100%>").arg(url.toString());
    tabPict->preview->setHtml(html, tabPict->preview->url());
}

