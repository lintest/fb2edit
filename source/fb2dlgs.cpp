#include "fb2dlgs.hpp"
#include "fb2code.hpp"
#include "fb2view.hpp"
#include "fb2utils.h"
#include "ui_fb2find.h"
#include "ui_fb2note.h"

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

Fb2TextFindDlg::Fb2TextFindDlg(Fb2WebView &edit)
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

Fb2NoteDlg::Fb2NoteDlg(Fb2WebView &view)
    : QDialog(&view)
    , ui(new Ui::Fb2Note)
{
    ui->setupUi(this);
    ui->m_key->addItem(tr("<create new>"));
    ui->m_key->setCurrentIndex(0);
    ui->m_title->setFocus();

    Fb2WebPage *page = new Fb2WebPage(this);
    page->setNetworkAccessManager(view.page()->networkAccessManager());
    page->setContentEditable(true);
    ui->m_text->setPage(page);
    ui->m_text->setHtml("<p></p>");

    FB2::addTools(ui->m_toolbar, ui->m_text);
}

Fb2NoteDlg::~Fb2NoteDlg()
{
    delete ui;
}
