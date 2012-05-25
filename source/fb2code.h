#ifndef FB2CODE_H
#define FB2CODE_H

#include <Qsci/qsciscintilla.h>

class Fb2Scintilla : public QsciScintilla
{
    Q_OBJECT
public:
    explicit Fb2Scintilla(QWidget *parent = 0);
    void load(const QByteArray &array);
    
signals:

public slots:

private slots:
    void linesChanged();
    
};

#endif // FB2CODE_H
