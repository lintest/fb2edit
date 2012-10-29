#include "fb2code.hpp"
#include "fb2dlgs.hpp"

#include <QXmlInputSource>
#include <QtGui>

static const QColor DEFAULT_SYNTAX_CHAR     = Qt::blue;
static const QColor DEFAULT_ELEMENT_NAME    = Qt::darkRed;
static const QColor DEFAULT_COMMENT         = Qt::darkGray;
static const QColor DEFAULT_ATTRIBUTE_NAME  = Qt::red;
static const QColor DEFAULT_ATTRIBUTE_VALUE = Qt::darkGreen;
static const QColor DEFAULT_ERROR           = Qt::darkMagenta;
static const QColor DEFAULT_OTHER           = Qt::black;

// Regular expressions for parsing XML borrowed from:
// http://www.cs.sfu.ca/~cameron/REX.html
static const QString EXPR_COMMENT			= "<!--[^-]*-([^-][^-]*-)*->";
static const QString EXPR_COMMENT_BEGIN		= "<!--";
static const QString EXPR_COMMENT_END		= "[^-]*-([^-][^-]*-)*->";
static const QString EXPR_ATTRIBUTE_VALUE	= "\"[^<\"]*\"|'[^<']*'";
static const QString EXPR_NAME				= "([A-Za-z_:]|[^\\x00-\\x7F])([A-Za-z0-9_:.-]|[^\\x00-\\x7F])*";

FbHighlighter::FbHighlighter(QObject* parent)
: QSyntaxHighlighter(parent)
{
    init();
}

FbHighlighter::FbHighlighter(QTextDocument* parent)
: QSyntaxHighlighter(parent)
{
    init();
}

FbHighlighter::FbHighlighter(QTextEdit* parent)
: QSyntaxHighlighter(parent)
{
    init();
}

FbHighlighter::~FbHighlighter()
{
}

void FbHighlighter::init()
{
    fmtSyntaxChar.setForeground(DEFAULT_SYNTAX_CHAR);
    fmtElementName.setForeground(DEFAULT_ELEMENT_NAME);
    fmtComment.setForeground(DEFAULT_COMMENT);
    fmtAttributeName.setForeground(DEFAULT_ATTRIBUTE_NAME);
    fmtAttributeValue.setForeground(DEFAULT_ATTRIBUTE_VALUE);
    fmtError.setForeground(DEFAULT_ERROR);
    fmtOther.setForeground(DEFAULT_OTHER);
}

void FbHighlighter::setHighlightColor(HighlightType type, QColor color, bool foreground)
{
    QTextCharFormat format;
    if (foreground)
        format.setForeground(color);
    else
        format.setBackground(color);
    setHighlightFormat(type, format);
}

void FbHighlighter::setHighlightFormat(HighlightType type, QTextCharFormat format)
{
    switch (type)
    {
        case SyntaxChar:
            fmtSyntaxChar = format;
            break;
        case ElementName:
            fmtElementName = format;
            break;
        case Comment:
            fmtComment = format;
            break;
        case AttributeName:
            fmtAttributeName = format;
            break;
        case AttributeValue:
            fmtAttributeValue = format;
            break;
        case Error:
            fmtError = format;
            break;
        case Other:
            fmtOther = format;
            break;
    }
    rehighlight();
}

void FbHighlighter::highlightBlock(const QString& text)
{
    int i = 0;
    int pos = 0;
    int brackets = 0;

    state = (previousBlockState() == InElement ? ExpectAttributeOrEndOfElement : NoState);

    if (previousBlockState() == InComment)
    {
        // search for the end of the comment
        QRegExp expression(EXPR_COMMENT_END);
        pos = expression.indexIn(text, i);

        if (pos >= 0)
        {
            // end comment found
            const int iLength = expression.matchedLength();
            setFormat(0, iLength - 3, fmtComment);
            setFormat(iLength - 3, 3, fmtSyntaxChar);
            i += iLength; // skip comment
        }
        else
        {
            // in comment
            setFormat(0, text.length(), fmtComment);
            setCurrentBlockState(InComment);
            return;
        }
    }

    for (; i < text.length(); i++)
    {
        switch (text.at(i).toAscii())
        {
        case '<':
            brackets++;
            if (brackets == 1)
            {
                setFormat(i, 1, fmtSyntaxChar);
                state = ExpectElementNameOrSlash;
            }
            else
            {
                // wrong bracket nesting
                setFormat(i, 1, fmtError);
            }
            break;

        case '>':
            brackets--;
            if (brackets == 0)
            {
                setFormat(i, 1, fmtSyntaxChar);
            }
            else
            {
                // wrong bracket nesting
                setFormat( i, 1, fmtError);
            }
            state = NoState;
            break;

        case '/':
            if (state == ExpectElementNameOrSlash)
            {
                state = ExpectElementName;
                setFormat(i, 1, fmtSyntaxChar);
            }
            else
            {
                if (state == ExpectAttributeOrEndOfElement)
                {
                    setFormat(i, 1, fmtSyntaxChar);
                }
                else
                {
                    processDefaultText(i, text);
                }
            }
            break;

        case '=':
            if (state == ExpectEqual)
            {
                state = ExpectAttributeValue;
                setFormat(i, 1, fmtOther);
            }
            else
            {
                processDefaultText(i, text);
            }
            break;

        case '\'':
        case '\"':
            if (state == ExpectAttributeValue)
            {
                // search attribute value
                QRegExp expression(EXPR_ATTRIBUTE_VALUE);
                pos = expression.indexIn(text, i);

                if (pos == i) // attribute value found ?
                {
                    const int iLength = expression.matchedLength();

                    setFormat(i, 1, fmtOther);
                    setFormat(i + 1, iLength - 2, fmtAttributeValue);
                    setFormat(i + iLength - 1, 1, fmtOther);

                    i += iLength - 1; // skip attribute value
                    state = ExpectAttributeOrEndOfElement;
                }
                else
                {
                    processDefaultText(i, text);
                }
            }
            else
            {
                processDefaultText(i, text);
            }
            break;

        case '!':
            if (state == ExpectElementNameOrSlash)
            {
                // search comment
                QRegExp expression(EXPR_COMMENT);
                pos = expression.indexIn(text, i - 1);

                if (pos == i - 1) // comment found ?
                {
                    const int iLength = expression.matchedLength();

                    setFormat(pos, 4, fmtSyntaxChar);
                    setFormat(pos + 4, iLength - 7, fmtComment);
                    setFormat(iLength - 3, 3, fmtSyntaxChar);
                    i += iLength - 2; // skip comment
                    state = NoState;
                    brackets--;
                }
                else
                {
                    // Try find multiline comment
                    QRegExp expression(EXPR_COMMENT_BEGIN); // search comment start
                    pos = expression.indexIn(text, i - 1);

                    //if (pos == i - 1) // comment found ?
                    if (pos >= i - 1)
                    {
                        setFormat(i, 3, fmtSyntaxChar);
                        setFormat(i + 3, text.length() - i - 3, fmtComment);
                        setCurrentBlockState(InComment);
                        return;
                    }
                    else
                    {
                        processDefaultText(i, text);
                    }
                }
            }
            else
            {
                processDefaultText(i, text);
            }

            break;

        default:
            const int iLength = processDefaultText(i, text);
            if (iLength > 0)
                i += iLength - 1;
            break;
        }
    }

    if (state == ExpectAttributeOrEndOfElement)
    {
        setCurrentBlockState(InElement);
    }
}

int FbHighlighter::processDefaultText(int i, const QString& text)
{
    // length of matched text
    int iLength = 0;

    switch(state)
    {
    case ExpectElementNameOrSlash:
    case ExpectElementName:
        {
            // search element name
            QRegExp expression(EXPR_NAME);
            const int pos = expression.indexIn(text, i);

            if (pos == i) // found ?
            {
                iLength = expression.matchedLength();

                setFormat(pos, iLength, fmtElementName);
                state = ExpectAttributeOrEndOfElement;
            }
            else
            {
                setFormat(i, 1, fmtOther);
            }
        }
        break;

    case ExpectAttributeOrEndOfElement:
        {
            // search attribute name
            QRegExp expression(EXPR_NAME);
            const int pos = expression.indexIn(text, i);

            if (pos == i) // found ?
            {
                iLength = expression.matchedLength();

                setFormat(pos, iLength, fmtAttributeName);
                state = ExpectEqual;
            }
            else
            {
                setFormat(i, 1, fmtOther);
            }
        }
        break;

    default:
        setFormat(i, 1, fmtOther);
        break;
    }
    return iLength;
}

qreal FbCodeEdit::baseFontSize = 10;
qreal FbCodeEdit::zoomRatioMin = 0.2;
qreal FbCodeEdit::zoomRatioMax = 5.0;

FbCodeEdit::FbCodeEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);
    highlighter = new FbHighlighter(this);
    highlighter->setDocument( document() );

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    zoomRatio = 1;

    #ifdef Q_WS_WIN
    setFont(QFont("Courier New", baseFontSize));
    #else
    setFont(QFont("Monospace", baseFontSize));
    #endif

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void FbCodeEdit::setAction(Fb::Actions index, QAction *action)
{
    m_actions[index] = action;
}

bool FbCodeEdit::read(QIODevice *device)
{
    QByteArray data = device->readAll();
    QXmlInputSource source;
    source.setData(data);
    setPlainText(source.data());
    return true;
}

int FbCodeEdit::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
}

void FbCodeEdit::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void FbCodeEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void FbCodeEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void FbCodeEdit::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void FbCodeEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

bool FbCodeEdit::findText(const QString &exp, QTextDocument::FindFlags options)
{
    return QPlainTextEdit::find(exp, options);
}

void FbCodeEdit::find()
{
    FbCodeFindDlg dlg(*this);
    dlg.exec();
}

void FbCodeEdit::zoomIn()
{
    qreal ratio = zoomRatio * 1.1;
    ratio = qMin(ratio, zoomRatioMax);
    setZoomRatio(ratio);
}

void FbCodeEdit::zoomOut()
{
    qreal ratio = zoomRatio / 1.1;
    ratio = qMax(ratio, zoomRatioMin);
    setZoomRatio(ratio);
}

void FbCodeEdit::zoomReset()
{
    setZoomRatio(1.0);
}

void FbCodeEdit::setZoomRatio(qreal ratio)
{
    if (!qFuzzyCompare(1 + zoomRatio, 1 + ratio)) {
        zoomRatio = ratio;
        QFont f = font();
        f.setPointSizeF(baseFontSize * zoomRatio);
        setFont(f);
    }
}
