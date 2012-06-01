#include "fb2note.hpp"
#include "fb2view.hpp"
#include "ui_fb2note.h"

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
    ui->m_text->setPage(page);
}

Fb2NoteDlg::~Fb2NoteDlg()
{
    delete ui;
}
