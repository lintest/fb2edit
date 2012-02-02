#include <QtGui>
#include <QtDebug>

#include "fb2main.h"
#include "fb2doc.h"

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerxml.h>

MainWindow::MainWindow()
{
    init();
    createText();
    setCurrentFile("");
}

MainWindow::MainWindow(const QString &filename)
{
    init();
    createQsci();
    loadXML(filename);
    setCurrentFile(filename);
}

MainWindow::MainWindow(const QString &filename, QTextDocument * document)
{
    init();
    createText();
    if (!document) document = loadFB2(filename);
    setCurrentFile(filename, document);
}

bool MainWindow::loadXML(const QString &filename)
{
    if (!filename.isEmpty()) {
        QFile file(filename);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            qsciEdit->clear();
            return qsciEdit->read(&file);
        }
    }
    return false;
}

Fb2MainDocument * MainWindow::loadFB2(const QString &filename)
{
    if (filename.isEmpty()) return NULL;

    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCritical() << tr("Cannot read file %1:\n%2.").arg(filename).arg(file.errorString());
        return NULL;
    }

    return Fb2MainDocument::load(file);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::fileNew()
{
    MainWindow *other = new MainWindow;
    other->move(x() + 40, y() + 40);
    other->show();
}

void MainWindow::fileOpen()
{
    QString filename = QFileDialog::getOpenFileName(this);
    if (filename.isEmpty()) return;

    MainWindow * existing = findMainWindow(filename);
    if (existing) {
        existing->show();
        existing->raise();
        existing->activateWindow();
        return;
    }

    if (textEdit) {
        QTextDocument * document = loadFB2(filename);
        if (!document) return;

        if (isUntitled && textEdit->document()->isEmpty() && !isWindowModified()) {
            setCurrentFile(filename, document);
        } else {
            MainWindow * other = new MainWindow(filename, document);
            other->move(x() + 40, y() + 40);
            other->show();
        }
    } else if (qsciEdit) {
        if (isUntitled && !isWindowModified()) {
            loadXML(filename);
            setCurrentFile(filename);
        } else {
            MainWindow * other = new MainWindow(filename);
            other->move(x() + 40, y() + 40);
            other->show();
        }

    }

}

bool MainWindow::fileSave()
{
    if (isUntitled) {
        return fileSaveAs();
    } else {
        return saveFile(curFile);
    }
}

bool MainWindow::fileSaveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), curFile);
    if (fileName.isEmpty()) return false;
    return saveFile(fileName);
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About SDI"),
            tr("The <b>SDI</b> example demonstrates how to write single "
               "document interface applications using Qt."));
}

void MainWindow::documentWasModified()
{
    QFileInfo info = windowFilePath();
    QString title = info.fileName();
    title += QString("[*]") += QString(" - ") += qApp->applicationName();
    setWindowTitle(title);
    setWindowModified(true);
}

void MainWindow::init()
{
    setAttribute(Qt::WA_DeleteOnClose);

    isUntitled = true;

    createActions();
    createStatusBar();

    readSettings();

    setUnifiedTitleAndToolBarOnMac(true);
}

void MainWindow::createActions()
{

//  http://svn.gnome.org/viewvc/gnome-icon-theme/trunk/24x24/actions/

    QIcon icon;
    QAction * act;
    QMenu * menu;
    QToolBar * tool;

    menu = menuBar()->addMenu(tr("&File"));
    tool = addToolBar(tr("File"));

    icon = QIcon::fromTheme("document-new", QIcon(":/images/new.png"));
    act = new QAction(icon, tr("&New"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::New);
    act->setStatusTip(tr("Create a new file"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileNew()));
    menu->addAction(act);
    tool->addAction(act);

    icon = QIcon::fromTheme("document-open", QIcon(":/images/open.png"));
    act = new QAction(icon, tr("&Open..."), this);
    act->setShortcuts(QKeySequence::Open);
    act->setStatusTip(tr("Open an existing file"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileOpen()));
    menu->addAction(act);
    tool->addAction(act);

    icon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
    act = new QAction(icon, tr("&Save"), this);
    act->setShortcuts(QKeySequence::Save);
    act->setStatusTip(tr("Save the document to disk"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileSave()));
    menu->addAction(act);
    tool->addAction(act);

    icon = QIcon::fromTheme("document-save-as");
    act = new QAction(icon, tr("Save &As..."), this);
    act->setShortcuts(QKeySequence::SaveAs);
    act->setStatusTip(tr("Save the document under a new name"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    menu->addAction(act);

    menu->addSeparator();

    icon = QIcon::fromTheme("window-close");
    act = new QAction(icon, tr("&Close"), this);
    act->setShortcuts(QKeySequence::Close);
    act->setStatusTip(tr("Close this window"));
    connect(act, SIGNAL(triggered()), this, SLOT(close()));
    menu->addAction(act);

    icon = QIcon::fromTheme("application-exit");
    act = new QAction(icon, tr("E&xit"), this);
    act->setShortcuts(QKeySequence::Quit);
    act->setStatusTip(tr("Exit the application"));
    connect(act, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
    menu->addAction(act);

    menu = menuBar()->addMenu(tr("&Edit"));
    tool = addToolBar(tr("Edit"));

    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));

    icon = QIcon::fromTheme("edit-undo", QIcon(":/images/editundo.png"));
    actionUndo = act = new QAction(icon, tr("&Undo"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Undo);
    menu->addAction(act);
    tool->addAction(act);

    icon = QIcon::fromTheme("edit-redo", QIcon(":/images/editredo.png"));
    actionRedo = act = new QAction(icon, tr("&Redo"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Redo);
    menu->addAction(act);
    tool->addAction(act);

    menu->addSeparator();
    tool->addSeparator();

    icon = QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png"));
    actionCut = act = new QAction(icon, tr("Cu&t"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Cut);
    act->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    act->setEnabled(false);
    menu->addAction(act);
    tool->addAction(act);

    icon = QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png"));
    actionCopy = act = new QAction(icon, tr("&Copy"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Copy);
    act->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    act->setEnabled(false);
    menu->addAction(act);
    tool->addAction(act);

    icon = QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png"));
    actionPaste = act = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Paste);
    act->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    menu->addAction(act);
    tool->addAction(act);
    clipboardDataChanged();

    menu = menuBar()->addMenu(tr("Format"));
    tool = addToolBar(tr("Format"));

    icon = QIcon::fromTheme("format-text-bold", QIcon(":/images/textbold.png"));
    actionTextBold = act = new QAction(icon, tr("Bold"), this);
    act->setShortcuts(QKeySequence::Bold);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), this, SLOT(textBold()));
    menu->addAction(act);
    tool->addAction(act);

    icon = QIcon::fromTheme("format-text-italic", QIcon(":/images/textitalic.png"));
    actionTextItalic = act = new QAction(icon, tr("Italic"), this);
    act->setShortcuts(QKeySequence::Italic);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), this, SLOT(textItalic()));
    menu->addAction(act);
    tool->addAction(act);

    icon = QIcon::fromTheme("format-text-underline", QIcon(":/images/textunderline.png"));
    actionTextUnder = act = new QAction(icon, tr("Underline"), this);
    act->setShortcuts(QKeySequence::Underline);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), this, SLOT(textUnder()));
    menu->addAction(act);
    tool->addAction(act);

    icon = QIcon::fromTheme("format-text-strikethrough", QIcon(":/images/textstrike.png"));
    actionTextStrike = act = new QAction(icon, tr("Strikethrough"), this);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), this, SLOT(textStrike()));
    menu->addAction(act);
    tool->addAction(act);

    // format-text-subscript
    // format-text-superscript

    menu = menuBar()->addMenu(tr("&View"));

    QAction * actText = act = new QAction(tr("&Text"), this);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), this, SLOT(viewText()));
    menu->addAction(act);

    QAction * actQsci = act = new QAction(tr("&XML"), this);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), this, SLOT(viewQsci()));
    menu->addAction(act);

    QActionGroup * viewGroup = new QActionGroup(this);
    viewGroup->addAction(actText);
    viewGroup->addAction(actQsci);
    actText->setChecked(true);

    menuBar()->addSeparator();

    menu = menuBar()->addMenu(tr("&Help"));

    icon = QIcon::fromTheme("help-about");
    act = new QAction(icon, tr("&About"), this);
    act->setStatusTip(tr("Show the application's About box"));
    connect(act, SIGNAL(triggered()), this, SLOT(about()));
    menu->addAction(act);

    act = new QAction(tr("About &Qt"), this);
    act->setStatusTip(tr("Show the Qt library's About box"));
    connect(act, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    menu->addAction(act);
}

void MainWindow::connectTextDocument(QTextDocument * document)
{
    connect(document, SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));
    connect(document, SIGNAL(undoAvailable(bool)), actionUndo, SLOT(setEnabled(bool)));
    connect(document, SIGNAL(redoAvailable(bool)), actionRedo, SLOT(setEnabled(bool)));
}

void MainWindow::createText()
{
    textEdit = new QTextEdit;
    textEdit->setAcceptRichText(true);
    setCentralWidget(textEdit);

    connect(actionCut, SIGNAL(triggered()), textEdit, SLOT(cut()));
    connect(actionCopy, SIGNAL(triggered()), textEdit, SLOT(copy()));
    connect(actionPaste, SIGNAL(triggered()), textEdit, SLOT(paste()));

    connect(textEdit, SIGNAL(copyAvailable(bool)), actionCut, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(copyAvailable(bool)), actionCopy, SLOT(setEnabled(bool)));

    connect(textEdit, SIGNAL(currentCharFormatChanged(const QTextCharFormat &)), this, SLOT(currentCharFormatChanged(const QTextCharFormat &)));

    connect(actionUndo, SIGNAL(triggered()), textEdit, SLOT(undo()));
    connect(actionRedo, SIGNAL(triggered()), textEdit, SLOT(redo()));

    actionUndo->setEnabled(false);
    actionRedo->setEnabled(false);

    connectTextDocument(textEdit->document());
}

void MainWindow::createQsci()
{
    //  http://qtcoder.blogspot.com/2010/10/qscintills.html
    //  http://www.riverbankcomputing.co.uk/static/Docs/QScintilla2/classQsciScintilla.html

    qsciEdit = new QsciScintilla;
    qsciEdit->setUtf8(true);
    qsciEdit->setCaretLineVisible(true);
    qsciEdit->setCaretLineBackgroundColor(QColor("gainsboro"));
    qsciEdit->setWrapMode(QsciScintilla::WrapWord);

    qsciEdit->setEolMode(QsciScintilla::EolWindows);

    qsciEdit->setAutoIndent(true);
    qsciEdit->setIndentationGuides(true);

    qsciEdit->setAutoCompletionSource(QsciScintilla::AcsAll);
    qsciEdit->setAutoCompletionCaseSensitivity(true);
    qsciEdit->setAutoCompletionReplaceWord(true);
    qsciEdit->setAutoCompletionShowSingle(true);
    qsciEdit->setAutoCompletionThreshold(2);

    qsciEdit->setMarginsBackgroundColor(QColor("gainsboro"));
    qsciEdit->setMarginWidth(0, 0);
    qsciEdit->setMarginLineNumbers(1, true);
    qsciEdit->setMarginWidth(1, QString("1000"));
    qsciEdit->setFolding(QsciScintilla::BoxedFoldStyle, 2);

    qsciEdit->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    qsciEdit->setMatchedBraceBackgroundColor(Qt::yellow);
    qsciEdit->setUnmatchedBraceForegroundColor(Qt::blue);

    QFont font("Courier", 10);
    font.setStyleHint(QFont::TypeWriter);

    QsciLexerXML * lexer = new QsciLexerXML;
    lexer->setFont(font, -1);

    qsciEdit->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    qsciEdit->setLexer(lexer);

    setCentralWidget(qsciEdit);
    qsciEdit->setFocus();

    //    connect(qsciEdit, SIGNAL(textChanged()), this, SLOT(documentWasModified()));
    //    connect(qsciEdit, SIGNAL(cursorPositionChanged(int, int)), this, SLOT(cursorMoved(int, int)));

    actionUndo->setEnabled(false);
    actionRedo->setEnabled(false);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QSettings settings;
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    move(pos);
    resize(size);
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

bool MainWindow::maybeSave()
{
    if (textEdit && textEdit->document()->isModified()) {
	QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("SDI"),
                     tr("The document has been modified.\n"
                        "Do you want to save your changes?"),
                     QMessageBox::Save | QMessageBox::Discard
		     | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return fileSave();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

bool MainWindow::saveFile(const QString &fileName)
{
    return false;

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("SDI"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << textEdit->toPlainText();
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}

void MainWindow::setCurrentFile(const QString &filename, QTextDocument * document)
{
    static int sequenceNumber = 1;

    QString title;
    isUntitled = filename.isEmpty();
    if (isUntitled) {
        curFile = tr("book%1.fb2").arg(sequenceNumber++);
        title = curFile;
    } else {
        QFileInfo info = filename;
        curFile = info.canonicalFilePath();
        title = info.fileName();
    }
    title += QString(" - ") += qApp->applicationName();

    if (textEdit && document) {
        textEdit->setDocument(document);
        textEdit->document()->setModified(false);
        connectTextDocument(textEdit->document());
    }

    setWindowModified(false);
    setWindowFilePath(curFile);
    setWindowTitle(title);
}

MainWindow *MainWindow::findMainWindow(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QWidget *widget, qApp->topLevelWidgets()) {
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin && mainWin->curFile == canonicalFilePath)
            return mainWin;
    }
    return 0;
}

void MainWindow::viewQsci()
{
    if (centralWidget() == qsciEdit) return;
    if (textEdit) { delete textEdit; textEdit = NULL; }
    createQsci();
}

void MainWindow::viewText()
{
    if (centralWidget() == textEdit) return;
    if (qsciEdit) { delete qsciEdit; qsciEdit = NULL; }
    createText();
}

void MainWindow::currentCharFormatChanged(const QTextCharFormat &format)
{
    actionTextBold   -> setChecked(format.font().bold());
    actionTextItalic -> setChecked(format.font().italic());
    actionTextUnder  -> setChecked(format.font().underline());
    actionTextStrike -> setChecked(format.font().strikeOut());
}

void MainWindow::cursorPositionChanged()
{
 //   alignmentChanged(textEdit->alignment());
}

void MainWindow::clipboardDataChanged()
{
    if (const QMimeData *md = QApplication::clipboard()->mimeData()) {
        actionPaste->setEnabled(md->hasText());
    }
}

void MainWindow::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void MainWindow::textUnder()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(actionTextUnder->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MainWindow::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MainWindow::textStrike()
{
    QTextCharFormat fmt;
    fmt.setFontStrikeOut(actionTextStrike->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MainWindow::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    if (!textEdit) return;
    QTextCursor cursor = textEdit->textCursor();
    if (!cursor.hasSelection()) cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    textEdit->mergeCurrentCharFormat(format);
}

