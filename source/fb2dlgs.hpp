#ifndef FB2DLGS_H
#define FB2DLGS_H

#include <QDialog>

class FbCodeEdit;
class FbTextBase;
class FbTextEdit;

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QTabWidget;
class QToolBar;
class QWebView;
QT_END_NAMESPACE

namespace Ui {
class FbFind;
class FbSetup;
}

class FbCodeFindDlg : public QDialog
{
    Q_OBJECT

public:
    explicit FbCodeFindDlg(FbCodeEdit &edit);
    virtual ~FbCodeFindDlg();

private slots:
    void find();

private:
    Ui::FbFind * ui;
    FbCodeEdit & m_edit;
};

class FbTextFindDlg : public QDialog
{
    Q_OBJECT

public:
    explicit FbTextFindDlg(FbTextEdit &edit);
    virtual ~FbTextFindDlg();

private slots:
    void find();

private:
    Ui::FbFind * ui;
    FbTextEdit & m_edit;
};

class FbNoteDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit FbNoteDlg(FbTextEdit &view);

private slots:
    void loadFinished();

private:
    QComboBox *m_key;
    FbTextBase *m_text;
    QLineEdit *m_title;
    QToolBar *m_toolbar;
};

class FbSetupDlg : public QDialog
{
    Q_OBJECT
public:
    explicit FbSetupDlg(QWidget *parent = 0);
private:
    Ui::FbSetup * ui;
};

class FbImageDlg : public QDialog
{
    Q_OBJECT

private:
    class FbTab: public QWidget
    {
    public:
        explicit FbTab(QWidget* parent);
        QLabel *label;
        QComboBox *combo;
        QWebView *preview;
    };

public:
    explicit FbImageDlg(QWidget *parent = 0);

private:
    QTabWidget *notebook;
    FbTab *tabPict;
    FbTab *tabFile;
};

#endif // FB2DLGS_H
