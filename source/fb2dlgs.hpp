#ifndef FB2DLGS_H
#define FB2DLGS_H

#include <QDialog>

class Fb2CodeEdit;
class Fb2WebView;

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QToolBar;
class QWebView;
QT_END_NAMESPACE

namespace Ui {
class Fb2Find;
}

class Fb2CodeFindDlg : public QDialog
{
    Q_OBJECT

public:
    explicit Fb2CodeFindDlg(Fb2CodeEdit &edit);
    virtual ~Fb2CodeFindDlg();

private slots:
    void find();

private:
    Ui::Fb2Find * ui;
    Fb2CodeEdit & m_edit;
};

class Fb2TextFindDlg : public QDialog
{
    Q_OBJECT

public:
    explicit Fb2TextFindDlg(Fb2WebView &edit);
    virtual ~Fb2TextFindDlg();

private slots:
    void find();

private:
    Ui::Fb2Find * ui;
    Fb2WebView & m_edit;
};

class Fb2NoteDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit Fb2NoteDlg(Fb2WebView &view);

private slots:
    void loadFinished();

private:
    QComboBox *m_key;
    QWebView *m_text;
    QLineEdit *m_title;
    QToolBar *m_toolbar;
};

#endif // FB2DLGS_H
