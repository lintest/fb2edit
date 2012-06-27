#include "fb2dlgs.hpp"
#include "fb2view.hpp"
#include "fb2utils.h"
#include "ui_fb2find.h"
#include "ui_fb2note.h"

//---------------------------------------------------------------------------
//  Fb2FindDlg
//---------------------------------------------------------------------------

Fb2FindDlg::Fb2FindDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Fb2Find)
{
    ui->setupUi(this);
}

Fb2FindDlg::~Fb2FindDlg()
{
    delete ui;
}

//---------------------------------------------------------------------------
//  Fb2NoteDlg
//---------------------------------------------------------------------------

Fb2NoteDlg::Fb2NoteDlg(Fb2WebView &view, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Fb2Note)
{
    ui->setupUi(this);
    ui->m_key->addItem(tr("<create new>"));
    ui->m_key->setCurrentIndex(0);
    ui->m_title->setFocus();

    Fb2WebPage *page = new Fb2WebPage(this);
    page->setNetworkAccessManager(view.page()->networkAccessManager());
    page->setContentEditable(true);
    ui->m_text->setPage(page);

    FB2::addTools(ui->m_toolbar, ui->m_text);
}

Fb2NoteDlg::~Fb2NoteDlg()
{
    delete ui;
}
