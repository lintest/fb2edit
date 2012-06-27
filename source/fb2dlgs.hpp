#ifndef FB2DLGS_H
#define FB2DLGS_H

#include <QDialog>

class Fb2WebView;

namespace Ui {
class Fb2Note;
class Fb2Find;
}

class Fb2NoteDlg : public QDialog
{
    Q_OBJECT
    
public:
    explicit Fb2NoteDlg(Fb2WebView &view, QWidget *parent = 0);
    virtual ~Fb2NoteDlg();
    
private:
    Ui::Fb2Note *ui;
};

class Fb2FindDlg : public QDialog
{
    Q_OBJECT

public:
    explicit Fb2FindDlg(QWidget *parent = 0);
    virtual ~Fb2FindDlg();

private:
    Ui::Fb2Find *ui;
};

#endif // FB2DLGS_H
