#include <QtGui>
#include <QtDebug>
#include <QTreeView>
#include <QWebFrame>

#include "fb2main.h"
#include "fb2read.h"
#include "fb2tree.h"
#include "fb2view.h"
#include "fb2main.h"

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerxml.h>

Fb2MainWindow::Fb2MainWindow()
{
    init();
    createText();
    setCurrentFile("");
}

Fb2MainWindow::Fb2MainWindow(const QString &filename)
{
    init();
    createQsci();
    loadXML(filename);
    setCurrentFile(filename);
}

Fb2MainWindow::Fb2MainWindow(const QString &filename, Fb2MainDocument * document)
{
    init();
    createText();
    thread = new Fb2ReadThread(this, filename);
    connect(thread, SIGNAL(sendDocument()), SLOT(sendDocument()));
    thread->start();
}

void Fb2MainWindow::init()
{
    thread = NULL;

    connect(qApp, SIGNAL(logMessage(QString)), SLOT(logMessage(QString)));

    setAttribute(Qt::WA_DeleteOnClose);

    isUntitled = true;

    createActions();
    createStatusBar();

    textEdit = NULL;
    noteEdit = NULL;
    qsciEdit = NULL;
    treeView = NULL;
    messageEdit = NULL;

    readSettings();

    setUnifiedTitleAndToolBarOnMac(true);
}

void Fb2MainWindow::logMessage(const QString &message)
{
    if (!messageEdit) {
        messageEdit = new QTextEdit(this);
        connect(messageEdit, SIGNAL(destroyed()), SLOT(logDestroyed()));
        QDockWidget * dock = new QDockWidget(tr("Message log"), this);
        dock->setAttribute(Qt::WA_DeleteOnClose);
        dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
        dock->setWidget(messageEdit);
        addDockWidget(Qt::BottomDockWidgetArea, dock);
        messageEdit->setMaximumHeight(80);
    }
    messageEdit->append(message);
}

void Fb2MainWindow::logShowed()
{
    messageEdit->setMaximumHeight(QWIDGETSIZE_MAX);
}

void Fb2MainWindow::logDestroyed()
{
    messageEdit = NULL;
}

static QTextFrame * findFrame(QTextFrame *root, const QModelIndex &index)
{
    if (!index.isValid()) return root;
    if (QTextFrame *frame = findFrame(root, index.parent())) {
        QTextFrame *child = static_cast<QTextFrame*>(index.internalPointer());
        int i = frame->childFrames().indexOf(child);
        if (i >= 0) return child;
    }
    return NULL;
}

void Fb2MainWindow::treeActivated(const QModelIndex &index)
{
}

void Fb2MainWindow::treeDestroyed()
{
    treeView = NULL;
}

bool Fb2MainWindow::loadXML(const QString &filename)
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

void Fb2MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void Fb2MainWindow::fileNew()
{
    Fb2MainWindow *other = new Fb2MainWindow;
    other->move(x() + 40, y() + 40);
    other->show();
}

void Fb2MainWindow::fileOpen()
{
    QString filename = QFileDialog::getOpenFileName(this);
    if (filename.isEmpty()) return;

    Fb2MainWindow * existing = findFb2MainWindow(filename);
    if (existing) {
        existing->show();
        existing->raise();
        existing->activateWindow();
        return;
    }

    if (textEdit) {
        if (isUntitled && !isWindowModified()) {
            textEdit->load(filename);
        } else {
            Fb2MainWindow * other = new Fb2MainWindow(filename, NULL);
            other->move(x() + 40, y() + 40);
            other->show();
        }
    } else if (qsciEdit) {
        if (isUntitled && !isWindowModified()) {
            loadXML(filename);
            setCurrentFile(filename);
        } else {
            Fb2MainWindow * other = new Fb2MainWindow(filename);
            other->move(x() + 40, y() + 40);
            other->show();
        }
    }
}
/*
void Fb2MainWindow::sendDocument()
{
    treeView = new QTreeView(this);
    treeView->setModel(new Fb2TreeModel(*textEdit));
    treeView->setHeaderHidden(true);
    connect(treeView, SIGNAL(activated(QModelIndex)), SLOT(treeActivated(QModelIndex)));
    connect(treeView, SIGNAL(destroyed()), SLOT(treeDestroyed()));
    QDockWidget * dock = new QDockWidget(tr("Contents"), this);
    dock->setAttribute(Qt::WA_DeleteOnClose);
    dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
    dock->setWidget(treeView);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
}
*/
bool Fb2MainWindow::fileSave()
{
    if (isUntitled) {
        return fileSaveAs();
    } else {
        return saveFile(curFile);
    }
}

bool Fb2MainWindow::fileSaveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), curFile);
    if (fileName.isEmpty()) return false;
    return saveFile(fileName);
}

void Fb2MainWindow::about()
{
   QMessageBox::about(this, tr("About SDI"),
            tr("The <b>SDI</b> example demonstrates how to write single "
               "document interface applications using Qt."));
}

void Fb2MainWindow::documentWasModified()
{
    QFileInfo info = windowFilePath();
    QString title = info.fileName();
    title += QString("[*]") += QString(" - ") += qApp->applicationName();
    setWindowTitle(title);
    setWindowModified(true);
}

QIcon Fb2MainWindow::icon(const QString &name)
{
    QIcon icon;
    icon.addFile(QString(":/res/24/%1.png").arg(name), QSize(24,24));
    icon.addFile(QString(":/res/16/%1.png").arg(name), QSize(16,16));
    return QIcon::fromTheme(name, icon);
}

void Fb2MainWindow::createActions()
{
    QAction * act;
    QMenu * menu;
    QToolBar * tool;
    QList<QAction*> actions;

    menu = menuBar()->addMenu(tr("&File"));
    tool = addToolBar(tr("File"));

    act = new QAction(icon("document-new"), tr("&New"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::New);
    act->setStatusTip(tr("Create a new file"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileNew()));
    menu->addAction(act);
    tool->addAction(act);

    act = new QAction(icon("document-open"), tr("&Open..."), this);
    act->setShortcuts(QKeySequence::Open);
    act->setStatusTip(tr("Open an existing file"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileOpen()));
    menu->addAction(act);
    tool->addAction(act);

    act = new QAction(icon("document-save"), tr("&Save"), this);
    act->setShortcuts(QKeySequence::Save);
    act->setStatusTip(tr("Save the document to disk"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileSave()));
    menu->addAction(act);
    tool->addAction(act);

    act = new QAction(icon("document-save-as"), tr("Save &As..."), this);
    act->setShortcuts(QKeySequence::SaveAs);
    act->setStatusTip(tr("Save the document under a new name"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(icon("window-close"), tr("&Close"), this);
    act->setShortcuts(QKeySequence::Close);
    act->setStatusTip(tr("Close this window"));
    connect(act, SIGNAL(triggered()), this, SLOT(close()));
    menu->addAction(act);

    act = new QAction(icon("application-exit"), tr("E&xit"), this);
    act->setShortcuts(QKeySequence::Quit);
    act->setStatusTip(tr("Exit the application"));
    connect(act, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
    menu->addAction(act);

    menu = menuBar()->addMenu(tr("&Edit"));
    tool = addToolBar(tr("Edit"));

    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));

    actionUndo = act = new QAction(icon("edit-undo"), tr("&Undo"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Undo);
    act->setEnabled(false);
    menu->addAction(act);
    tool->addAction(act);

    actionRedo = act = new QAction(icon("edit-redo"), tr("&Redo"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Redo);
    act->setEnabled(false);
    menu->addAction(act);
    tool->addAction(act);

    menu->addSeparator();
    tool->addSeparator();

    actionCut = act = new QAction(icon("edit-cut"), tr("Cu&t"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Cut);
    act->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    act->setEnabled(false);
    menu->addAction(act);
    tool->addAction(act);

    actionCopy = act = new QAction(icon("edit-copy"), tr("&Copy"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Copy);
    act->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    act->setEnabled(false);
    menu->addAction(act);
    tool->addAction(act);

    actionPaste = act = new QAction(icon("edit-paste"), tr("&Paste"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Paste);
    act->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    menu->addAction(act);
    tool->addAction(act);
    clipboardDataChanged();

    menu = menuBar()->addMenu(tr("Format"));
    tool = addToolBar(tr("Format"));

    actionTextBold = act = new QAction(icon("format-text-bold"), tr("Bold"), this);
    act->setShortcuts(QKeySequence::Bold);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), SLOT(textBold()));
    menu->addAction(act);
    tool->addAction(act);

    actionTextItalic = act = new QAction(icon("format-text-italic"), tr("Italic"), this);
    act->setShortcuts(QKeySequence::Italic);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), SLOT(textItalic()));
    menu->addAction(act);
    tool->addAction(act);

    actionTextStrike = act = new QAction(icon("format-text-strikethrough"), tr("Strikethrough"), this);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), SLOT(textStrike()));
    menu->addAction(act);
    tool->addAction(act);

    actionTextSup = act = new QAction(icon("format-text-superscript"), tr("Superscript"), this);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), SLOT(textSup()));
    menu->addAction(act);
    tool->addAction(act);

    actionTextSub = act = new QAction(icon("format-text-subscript"), tr("Subscript"), this);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), SLOT(textSub()));
    menu->addAction(act);
    tool->addAction(act);

    menu = menuBar()->addMenu(tr("&View"));

    QAction * actText = act = new QAction(tr("&Text"), this);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), this, SLOT(viewText()));

    QAction * actQsci = act = new QAction(tr("&XML"), this);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), this, SLOT(viewQsci()));

    QActionGroup * viewGroup = new QActionGroup(this);
    viewGroup->addAction(actText);
    viewGroup->addAction(actQsci);
    actText->setChecked(true);

    menu->addAction(actText);
    menu->addAction(actQsci);
    menu->addSeparator();

    tool = addToolBar(tr("Zoom"));

    actionZoomIn = act = new QAction(icon("zoom-in"), tr("Zoom in"), this);
    act->setShortcuts(QKeySequence::ZoomIn);
    menu->addAction(act);
    tool->addAction(act);

    actionZoomOut = act = new QAction(icon("zoom-out"), tr("Zoom out"), this);
    act->setShortcuts(QKeySequence::ZoomOut);
    menu->addAction(act);
    tool->addAction(act);

    actionZoomOrig = act = new QAction(icon("zoom-original"), tr("Zoom original"), this);
    menu->addAction(act);
    tool->addAction(act);

    menuBar()->addSeparator();

    menu = menuBar()->addMenu(tr("&Help"));

    act = new QAction(icon("help-about"), tr("&About"), this);
    act->setStatusTip(tr("Show the application's About box"));
    connect(act, SIGNAL(triggered()), this, SLOT(about()));
    menu->addAction(act);

    act = new QAction(tr("About &Qt"), this);
    act->setStatusTip(tr("Show the Qt library's About box"));
    connect(act, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    menu->addAction(act);
}
/*
void Fb2MainWindow::connectTextDocument(QTextDocument * document)
{
    connect(document, SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));
    connect(document, SIGNAL(undoAvailable(bool)), actionUndo, SLOT(setEnabled(bool)));
    connect(document, SIGNAL(redoAvailable(bool)), actionRedo, SLOT(setEnabled(bool)));
}
*/
void Fb2MainWindow::createText()
{
    textEdit = new Fb2WebView(this);
    setCentralWidget(textEdit);
    textEdit->setFocus();

    connect(textEdit, SIGNAL(selectionChanged()), this, SLOT(contentChanged()));

    connect(actionUndo, SIGNAL(triggered()), textEdit->pageAction(QWebPage::Undo), SIGNAL(triggered()));
    connect(actionRedo, SIGNAL(triggered()), textEdit->pageAction(QWebPage::Redo), SIGNAL(triggered()));

    connect(textEdit->pageAction(QWebPage::Undo), SIGNAL(changed()), this, SLOT(undoChanged()));
    connect(textEdit->pageAction(QWebPage::Redo), SIGNAL(changed()), this, SLOT(redoChanged()));

    connect(actionCut, SIGNAL(triggered()), textEdit->pageAction(QWebPage::Cut), SIGNAL(triggered()));
    connect(actionCopy, SIGNAL(triggered()), textEdit->pageAction(QWebPage::Copy), SIGNAL(triggered()));
    connect(actionPaste, SIGNAL(triggered()), textEdit->pageAction(QWebPage::Paste), SIGNAL(triggered()));

    connect(actionTextBold, SIGNAL(triggered()), textEdit->pageAction(QWebPage::ToggleBold), SIGNAL(triggered()));
    connect(actionTextItalic, SIGNAL(triggered()), textEdit->pageAction(QWebPage::ToggleItalic), SIGNAL(triggered()));
    connect(actionTextStrike, SIGNAL(triggered()), textEdit->pageAction(QWebPage::ToggleStrikethrough), SIGNAL(triggered()));
    connect(actionTextSub, SIGNAL(triggered()), textEdit->pageAction(QWebPage::ToggleSubscript), SIGNAL(triggered()));
    connect(actionTextSup, SIGNAL(triggered()), textEdit->pageAction(QWebPage::ToggleSuperscript), SIGNAL(triggered()));

    connect(actionZoomIn, SIGNAL(triggered()), textEdit, SLOT(zoomIn()));
    connect(actionZoomOut, SIGNAL(triggered()), textEdit, SLOT(zoomOut()));
    connect(actionZoomOrig, SIGNAL(triggered()), textEdit, SLOT(zoomOrig()));
}

void Fb2MainWindow::selectionChanged()
{
    actionCut->setEnabled(textEdit->CutEnabled());
    actionCopy->setEnabled(textEdit->CopyEnabled());

    actionTextBold->setChecked(textEdit->BoldChecked());
    actionTextItalic->setChecked(textEdit->ItalicChecked());
    actionTextStrike->setChecked(textEdit->StrikeChecked());
    actionTextSub->setChecked(textEdit->SubChecked());
    actionTextSup->setChecked(textEdit->SupChecked());
}

void Fb2MainWindow::undoChanged()
{
    actionUndo->setEnabled(textEdit->UndoEnabled());
}

void Fb2MainWindow::redoChanged()
{
    actionRedo->setEnabled(textEdit->RedoEnabled());
}

void Fb2MainWindow::createQsci()
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
    qsciEdit->setMarginWidth(1, QString("10000"));
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

    actionUndo->setEnabled(false);
    actionRedo->setEnabled(false);
}

void Fb2MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void Fb2MainWindow::readSettings()
{
    QSettings settings;
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    move(pos);
    resize(size);
}

void Fb2MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

bool Fb2MainWindow::maybeSave()
{
    if (textEdit && textEdit->isModified()) {
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

bool Fb2MainWindow::saveFile(const QString &fileName)
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
//    out << textEdit->toPlainText();
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}

void Fb2MainWindow::setCurrentFile(const QString &filename, const QString &html)
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

    textEdit->setHtml(html, QUrl("fb2://s/"));

    setWindowModified(false);
    setWindowFilePath(curFile);
    setWindowTitle(title);
}

Fb2MainWindow *Fb2MainWindow::findFb2MainWindow(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QWidget *widget, qApp->topLevelWidgets()) {
        Fb2MainWindow *mainWin = qobject_cast<Fb2MainWindow *>(widget);
        if (mainWin && mainWin->curFile == canonicalFilePath)
            return mainWin;
    }
    return 0;
}

void Fb2MainWindow::viewQsci()
{
    if (centralWidget() == qsciEdit) return;
    QString html;
    if (textEdit) {
        html = textEdit->page()->mainFrame()->toHtml();
        delete textEdit;
        textEdit = NULL;
    }
    createQsci();
    qsciEdit->setText(html);
}

void Fb2MainWindow::viewText()
{
    if (centralWidget() == textEdit) return;
    if (qsciEdit) { delete qsciEdit; qsciEdit = NULL; }
    createText();
}

void Fb2MainWindow::clipboardDataChanged()
{
    if (const QMimeData *md = QApplication::clipboard()->mimeData()) {
        actionPaste->setEnabled(md->hasText());
    }
}

void Fb2MainWindow::textBold()
{
}

void Fb2MainWindow::textItalic()
{
}

void Fb2MainWindow::textStrike()
{
}

void Fb2MainWindow::textSub()
{
}

void Fb2MainWindow::textSup()
{
}

