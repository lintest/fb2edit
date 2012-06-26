#ifndef FB2CODE_H
#define FB2CODE_H

#ifdef FB2_USE_SCINTILLA

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
    void zoomReset();

private slots:
    void linesChanged();

};

#else // FB2_USE_SCINTILLA
    #ifndef FB2_USE_PLAINTEXT
        #define FB2_USE_PLAINTEXT
    #endif // FB2_USE_PLAINTEXT
#endif // FB2_USE_SCINTILLA

#ifdef FB2_USE_PLAINTEXT

#include <QByteArray>
#include <QList>
#include <QPlainTextEdit>
#include <QObject>

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QColor>
#include <QTextEdit>

class Fb2Highlighter : public QSyntaxHighlighter
{
public:
    Fb2Highlighter(QObject* parent);
    Fb2Highlighter(QTextDocument* parent);
    Fb2Highlighter(QTextEdit* parent);
    ~Fb2Highlighter();

    enum HighlightType
    {
        SyntaxChar,
        ElementName,
        Comment,
        AttributeName,
        AttributeValue,
        Error,
        Other
    };

    void setHighlightColor(HighlightType type, QColor color, bool foreground = true);
    void setHighlightFormat(HighlightType type, QTextCharFormat format);

protected:
    void highlightBlock(const QString& rstrText);
    int  processDefaultText(int i, const QString& rstrText);

private:
    void init();

    QTextCharFormat fmtSyntaxChar;
    QTextCharFormat fmtElementName;
    QTextCharFormat fmtComment;
    QTextCharFormat fmtAttributeName;
    QTextCharFormat fmtAttributeValue;
    QTextCharFormat fmtError;
    QTextCharFormat fmtOther;

    enum ParsingState
    {
        NoState = 0,
        ExpectElementNameOrSlash,
        ExpectElementName,
        ExpectAttributeOrEndOfElement,
        ExpectEqual,
        ExpectAttributeValue
    };

    enum BlockState
    {
        NoBlock = -1,
        InComment,
        InElement
    };

    ParsingState state;
};

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
QT_END_NAMESPACE

class Fb2CodeEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    Fb2CodeEdit(QWidget *parent = 0);

    QString text() const { return toPlainText(); }

    bool read(QIODevice *device)
        { Q_UNUSED(device); return true; }

    void load(const QByteArray data, const QList<int> folds)
        { setPlainText(QString::fromUtf8(data.data())); }

    bool isUndoAvailable() { return false; }

    bool isRedoAvailable() { return false; }

    void zoomTo ( int size ) {}

public slots:
    void zoomIn();
    void zoomOut();
    void zoomReset();

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
    void setZoomRatio(qreal ratio);

private:
    Fb2Highlighter * highlighter;
    QWidget *lineNumberArea;
    qreal zoomRatio;
    static qreal baseFontSize;
    static qreal zoomRatioMin;
    static qreal zoomRatioMax;
    friend class Fb2CodeEdit::LineNumberArea;
};

#endif // FB2_USE_PLAINTEXT

#endif // FB2CODE_H
