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
//  FbSetupDlg
//---------------------------------------------------------------------------

FbSetupDlg::FbSetupDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FbSetup)
{
    ui->setupUi(this);
}
