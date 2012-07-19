#ifndef FB2CODE_H
#define FB2CODE_H

#include <QByteArray>
#include <QList>
#include <QPlainTextEdit>
#include <QObject>

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QColor>
#include <QTextEdit>

class FbHighlighter : public QSyntaxHighlighter
{
public:
    FbHighlighter(QObject* parent);
    FbHighlighter(QTextDocument* parent);
    FbHighlighter(QTextEdit* parent);
    ~FbHighlighter();

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

class FbCodeEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    FbCodeEdit(QWidget *parent = 0);

    QString text() const { return toPlainText(); }

    bool read(QIODevice *device);

    void load(const QByteArray data, const QList<int> folds)
        { setPlainText(QString::fromUtf8(data.data())); }

    bool isUndoAvailable() { return false; }

    bool isRedoAvailable() { return false; }

    void zoomTo ( int size ) {}

    bool findText(const QString &exp, QTextDocument::FindFlags options = 0);

    bool isModified() const { return document()->isModified(); }

public slots:
    void find();
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
        LineNumberArea(FbCodeEdit *parent) : QWidget(parent) { editor = parent; }
        QSize sizeHint() const { return QSize(editor->lineNumberAreaWidth(), 0); }
    protected:
        void paintEvent(QPaintEvent *event) { editor->lineNumberAreaPaintEvent(event); }
    private:
        FbCodeEdit *editor;
    };

private:
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void setZoomRatio(qreal ratio);

private:
    FbHighlighter * highlighter;
    QWidget *lineNumberArea;
    qreal zoomRatio;
    static qreal baseFontSize;
    static qreal zoomRatioMin;
    static qreal zoomRatioMax;
    friend class FbCodeEdit::LineNumberArea;
};

#endif // FB2CODE_H
