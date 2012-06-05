#ifndef FB2CODE_H
#define FB2CODE_H

#ifdef USE_SCINTILLA

#include <Qsci/qsciscintilla.h>
#include <QList>

class Fb2CodeEdit : public QsciScintilla
{
    Q_OBJECT
public:
    explicit Fb2CodeEdit(QWidget *parent = 0);
    void load(const QByteArray &array, const QList<int> &folds);
    
signals:

public slots:

private slots:
    void linesChanged();

};

#else // USE_SCINTILLA

#include <QByteArray>
#include <QList>
#include <QPlainTextEdit>
#include <QObject>

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
QT_END_NAMESPACE

class LineNumberArea;

class Fb2CodeEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    Fb2CodeEdit(QWidget *parent = 0);

    QString text() const { return toPlainText(); }

    bool read(QIODevice *device) { return true; }

    void zoomTo ( int size ) {}

    void load(const QByteArray data, const QList<int> folds)
        { setPlainText(QString::fromUtf8(data.data())); }

    bool isUndoAvailable() { return false; }

    bool isRedoAvailable() { return false; }

public slots:
    void zoomIn() {}
    void zoomOut() {}

protected:
    void resizeEvent(QResizeEvent *event);

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    class LineNumberArea : public QWidget
    {
    public:
        LineNumberArea(Fb2CodeEdit *parent) : QWidget(parent) { editor = parent; }
        QSize sizeHint() const { return QSize(editor->lineNumberAreaWidth(), 0); }
    protected:
        void paintEvent(QPaintEvent *event) { editor->lineNumberAreaPaintEvent(event); }
    private:
        Fb2CodeEdit *editor;
    };

private:
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    QWidget *lineNumberArea;
    friend class Fb2CodeEdit::LineNumberArea;
};

typedef Fb2CodeEdit Fb2CodeEdit;

#endif // USE_SCINTILLA

#endif // FB2CODE_H
