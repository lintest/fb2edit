#include "fb2code.h"

/////////////////////////////////////////////////////////////////////////////
//
//  http://qtcoder.blogspot.com/2010/10/qscintills.html
//  http://www.riverbankcomputing.co.uk/static/Docs/QScintilla2/classQsciScintilla.html
//
/////////////////////////////////////////////////////////////////////////////

#include <Qsci/qscilexerxml.h>

Fb2Scintilla::Fb2Scintilla(QWidget *parent) :
    QsciScintilla(parent)
{
    zoomTo(1);
    setUtf8(true);
    setCaretLineVisible(true);
    setCaretLineBackgroundColor(QColor("gainsboro"));
    setWrapMode(QsciScintilla::WrapWord);

    setEolMode(QsciScintilla::EolWindows);

    setAutoIndent(true);
    setIndentationGuides(true);

    setAutoCompletionSource(QsciScintilla::AcsAll);
    setAutoCompletionCaseSensitivity(true);
    setAutoCompletionReplaceWord(true);
    setAutoCompletionShowSingle(true);
    setAutoCompletionThreshold(2);

    setMarginsBackgroundColor(QColor("gainsboro"));
    setMarginLineNumbers(0, true);
    setFolding(QsciScintilla::BoxedFoldStyle, 1);

    setBraceMatching(QsciScintilla::SloppyBraceMatch);
    setMatchedBraceBackgroundColor(Qt::yellow);
    setUnmatchedBraceForegroundColor(Qt::blue);

    QFont font("Courier", 10);
    font.setStyleHint(QFont::TypeWriter);

    QsciLexerXML * lexer = new QsciLexerXML;
    lexer->setFont(font, -1);

    setBraceMatching(QsciScintilla::SloppyBraceMatch);
    setLexer(lexer);

    connect(this, SIGNAL(linesChanged()), SLOT(linesChanged()));
}

void Fb2Scintilla::linesChanged()
{
    QString width = QString().setNum(lines() * 10);
    setMarginWidth(0, width);
}

