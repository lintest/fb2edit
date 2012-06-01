#ifndef FB2NOTE_H
#define FB2NOTE_H

#include <QDialog>

class Fb2WebView;

namespace Ui {
class Fb2Note;
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

#endif // FB2NOTE_H
