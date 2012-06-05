#include "fb2code.hpp"

#ifdef USE_SCINTILLA

/////////////////////////////////////////////////////////////////////////////
//
//  http://qtcoder.blogspot.com/2010/10/qscintills.html
//  http://www.riverbankcomputing.co.uk/static/Docs/QScintilla2/classQsciScintilla.html
//
/////////////////////////////////////////////////////////////////////////////

#include <Qsci/qscilexerxml.h>
#include <QtDebug>

Fb2CodeEdit::Fb2CodeEdit(QWidget *parent) :
    QsciScintilla(parent)
{
    zoomTo(1);
    setUtf8(true);
    setCaretLineVisible(true);
    setCaretLineBackgroundColor(QColor("gainsboro"));
    setWrapMode(QsciScintilla::WrapWord);

    /*
    //setup autocompletion
    setAutoCompletionSource(QsciScintilla::AcsAll);
    setAutoCompletionCaseSensitivity(true);
    setAutoCompletionReplaceWord(true);
    setAutoCompletionShowSingle(true);
    setAutoCompletionThreshold(2);
    */

    //setup margins
    setMarginsBackgroundColor(QColor("gainsboro"));
    setMarginLineNumbers(0, true);
    setFolding(QsciScintilla::BoxedFoldStyle, 1);

    //setup brace matching
    setBraceMatching(QsciScintilla::SloppyBraceMatch);
    setMatchedBraceBackgroundColor(Qt::yellow);
    setUnmatchedBraceForegroundColor(Qt::blue);

    //setup end-of-line mode
    #if defined Q_WS_X11
    setEolMode(QsciScintilla::EolUnix);
    #elif defined Q_WS_WIN
    setEolMode(QsciScintilla::EolWindows);
    #elif defined Q_WS_MAC
    setEolMode(QsciScintilla::EolMac);
    #endif

    //setup auto-indentation
    setAutoIndent(true);
    setIndentationGuides(true);
    setIndentationsUseTabs(false);
    setIndentationWidth(2);

    QsciLexerXML * lexer = new QsciLexerXML;
    lexer->setFoldPreprocessor(true);

    #ifdef Q_WS_WIN
    lexer->setFont(QFont("Courier New", 8));
    #else
    lexer->setFont(QFont("Monospace", 8));
    #endif

    setLexer(lexer);

    connect(this, SIGNAL(linesChanged()), SLOT(linesChanged()));
}

void Fb2CodeEdit::linesChanged()
{
    QString width = QString().setNum(lines() * 10);
    setMarginWidth(0, width);
}

void Fb2CodeEdit::load(const QByteArray &array, const QList<int> &folds)
{
    SendScintilla(SCI_SETTEXT, array.constData());
    SendScintilla(SCI_EMPTYUNDOBUFFER);
    foldAll(false);
    foldLine(1);
    for (QList<int>::const_iterator it = folds.constBegin(); it != folds.constEnd(); it++) {
        foldLine(*it);
    }
}

#else // USE_SCINTILLA

#include <QtGui>

Fb2CodeEdit::Fb2CodeEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    #ifdef Q_WS_WIN
    setFont(QFont("Courier New", 8));
    #else
    setFont(QFont("Monospace", 8));
    #endif

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int Fb2CodeEdit::lineNumberAreaWidth()
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

void Fb2CodeEdit::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void Fb2CodeEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void Fb2CodeEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void Fb2CodeEdit::highlightCurrentLine()
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

void Fb2CodeEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
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

#endif // USE_SCINTILLA
