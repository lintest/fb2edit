#include "fb2main.hpp"

#include <QtGui>
#include <QtDebug>
#include <QTreeView>
#include <QWebFrame>

#include "fb2app.hpp"
#include "fb2logs.hpp"
#include "fb2code.hpp"
#include "fb2dlgs.hpp"
#include "fb2dock.hpp"
#include "fb2logs.hpp"
#include "fb2save.hpp"
#include "fb2text.hpp"
#include "fb2utils.h"

//---------------------------------------------------------------------------
//  FbMainWindow
//---------------------------------------------------------------------------

FbMainWindow::FbMainWindow(const QString &filename, ViewMode mode)
    : QMainWindow()
    , noteEdit(0)
    , toolEdit(0)
    , logDock(0)
    , isSwitched(false)
    , isUntitled(true)
{
    Q_UNUSED(mode);
    connect(qApp, SIGNAL(logMessage(QtMsgType, QString)), SLOT(logMessage(QtMsgType, QString)));

    setUnifiedTitleAndToolBarOnMac(true);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(QIcon(":icon.ico"));

    mainDock = new FbMainDock(this);
    connect(mainDock, SIGNAL(modificationChanged(bool)), SLOT(textChanged(bool)));
    setCentralWidget(mainDock);

    createActions();
    createStatusBar();
    readSettings();

    QString filepath = filename.isEmpty() ? QString(":blank.fb2") : filename;

    mainDock->setMode(Fb::Text);
    setCurrentFile(filename);
    mainDock->load(filepath);
}

void FbMainWindow::warning(int row, int col, const QString &msg)
{
    Q_UNUSED(row);
    Q_UNUSED(col);
    logMessage(QtWarningMsg, msg.simplified());
}

void FbMainWindow::error(int row, int col, const QString &msg)
{
    Q_UNUSED(row);
    Q_UNUSED(col);
    logMessage(QtCriticalMsg, msg.simplified());
}

void FbMainWindow::fatal(int row, int col, const QString &msg)
{
    Q_UNUSED(row);
    Q_UNUSED(col);
    logMessage(QtFatalMsg, msg.simplified());
}

void FbMainWindow::logMessage(QtMsgType type, const QString &message)
{
    if (!logDock) {
        logDock = new FbLogDock(tr("Message log"), this);
        connect(logDock, SIGNAL(destroyed()), SLOT(logDestroyed()));
        addDockWidget(Qt::BottomDockWidgetArea, logDock);
    }
    logDock->append(type, message);
}

void FbMainWindow::logDestroyed()
{
    logDock = NULL;
}

void FbMainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void FbMainWindow::fileNew()
{
    FbMainWindow *other = new FbMainWindow;
    other->move(x() + 40, y() + 40);
    other->show();
}

void FbMainWindow::fileOpen()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open file"), QString(), "Fiction book files (*.fb2)");
    if (filename.isEmpty()) {
        return;
    }

    FbMainWindow * existing = findFbMainWindow(filename);
    if (existing) {
        existing->show();
        existing->raise();
        existing->activateWindow();
        return;
    }

    if (isUntitled && !isWindowModified()) {
        mainDock->load(filename);
        setCurrentFile(filename);
        isUntitled = false;
    } else {
        FbMainWindow * other = new FbMainWindow(filename, FB2);
        other->mainDock->load(filename);
        other->move(x() + 40, y() + 40);
        other->show();
    }
}

bool FbMainWindow::fileSave()
{
    if (isUntitled) {
        bool success = fileSaveAs();
        isUntitled = success;
        return success;
    } else {
        return saveFile(curFile);
    }
}

bool FbMainWindow::fileSaveAs()
{
    FbSaveDialog dlg(this, tr("Save As..."));
    dlg.selectFile(curFile);
    if (!dlg.exec()) return false;
    QString fileName = dlg.fileName();
    if (fileName.isEmpty()) return false;
    return saveFile(fileName, dlg.codec());
}

void FbMainWindow::about()
{
    QString text = tr("<b>fb2edit</b> is an application for creating and editing FB2-files.");
    text += QString("<br>") += QString("<br>") += FbApplication::lastCommit();
    QMessageBox::about(this, tr("About fb2edit"), text);
}

void FbMainWindow::textChanged(bool modified)
{
    QFileInfo info = windowFilePath();
    QString title = info.fileName();
    if (modified) title += QString("[*]");
    title += appTitle();
    setWindowTitle(title);
    setWindowModified(modified);
}

void FbMainWindow::createActions()
{
    QAction * act;
    QMenu * menu;
    QToolBar * tool;

    FbTextEdit *text = mainDock->text();
    FbHeadEdit *head = mainDock->head();
    FbCodeEdit *code = mainDock->code();
    Q_UNUSED(head)

    menu = menuBar()->addMenu(tr("&File"));
    tool = addToolBar(tr("File"));
    tool->setIconSize(QSize(24, 24));
    tool->setMovable(false);

    act = new QAction(FbIcon("document-new"), tr("&New"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::New);
    act->setStatusTip(tr("Create a new file"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileNew()));
    menu->addAction(act);
    tool->addAction(act);

    act = new QAction(FbIcon("document-open"), tr("&Open..."), this);
    act->setShortcuts(QKeySequence::Open);
    act->setStatusTip(tr("Open an existing file"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileOpen()));
    menu->addAction(act);
    tool->addAction(act);

    act = new QAction(FbIcon("document-save"), tr("&Save"), this);
    act->setShortcuts(QKeySequence::Save);
    act->setStatusTip(tr("Save the document to disk"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileSave()));
    menu->addAction(act);
    tool->addAction(act);

    act = new QAction(FbIcon("document-save-as"), tr("Save &As..."), this);
    act->setShortcuts(QKeySequence::SaveAs);
    act->setStatusTip(tr("Save the document under a new name"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    menu->addAction(act);

#ifdef QT_DEBUG
    act = new QAction(tr("&Export HTML"), this);
    connect(act, SIGNAL(triggered()), text, SLOT(exportHtml()));
    menu->addAction(act);
#endif // QT_DEBUG

    menu->addSeparator();

    act = new QAction(FbIcon("window-close"), tr("&Close"), this);
    act->setShortcuts(QKeySequence::Close);
    act->setStatusTip(tr("Close this window"));
    connect(act, SIGNAL(triggered()), this, SLOT(close()));
    menu->addAction(act);

    act = new QAction(FbIcon("application-exit"), tr("E&xit"), this);
    act->setShortcuts(QKeySequence::Quit);
    act->setStatusTip(tr("Exit the application"));
    connect(act, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
    menu->addAction(act);

    menu = menuBar()->addMenu(tr("&Edit"));

    act = new FbTextAction(FbIcon("edit-undo"), tr("&Undo"), QWebPage::Undo, text);
    text->setAction(Fb::EditUndo, act);
    code->setAction(Fb::EditUndo, act);
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Undo);
    act->setEnabled(false);
    menu->addAction(act);

    act = new FbTextAction(FbIcon("edit-redo"), tr("&Redo"), QWebPage::Redo, text);
    text->setAction(Fb::EditRedo, act);
    code->setAction(Fb::EditRedo, act);
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Redo);
    act->setEnabled(false);
    menu->addAction(act);

    menu->addSeparator();

    act = new FbTextAction(FbIcon("edit-cut"), tr("Cu&t"), QWebPage::Cut, text);
    text->setAction(Fb::EditCut, act);
    code->setAction(Fb::EditCut, act);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Cut);
    act->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    act->setEnabled(false);
    menu->addAction(act);

    act = new FbTextAction(FbIcon("edit-copy"), tr("&Copy"), QWebPage::Copy, text);
    text->setAction(Fb::EditCopy, act);
    code->setAction(Fb::EditCopy, act);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Copy);
    act->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    act->setEnabled(false);
    menu->addAction(act);

    act = new FbTextAction(FbIcon("edit-paste"), tr("&Paste"), QWebPage::Paste, text);
    text->setAction(Fb::EditPaste, act);
    code->setAction(Fb::EditPaste, act);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Paste);
    act->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    menu->addAction(act);

    act = new FbTextAction(tr("Paste (no style)"), QWebPage::PasteAndMatchStyle, text);
    text->setAction(Fb::PasteText, act);
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(FbIcon("edit-find"), tr("&Find..."), this);
    text->setAction(Fb::EditFind, act);
    code->setAction(Fb::EditFind, act);
    act->setShortcuts(QKeySequence::Find);
    menu->addAction(act);

    act = new QAction(FbIcon("edit-find-replace"), tr("&Replace..."), this);
    text->setAction(Fb::EditReplace, act);
    code->setAction(Fb::EditReplace, act);
    menu->addAction(act);

    act = new QAction(FbIcon("tools-check-spelling"), tr("&Check..."), this);
    code->setAction(Fb::CheckText, act);
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(FbIcon("preferences-desktop"), tr("&Settings"), this);
    act->setShortcuts(QKeySequence::Preferences);
    act->setStatusTip(tr("Application settings"));
    connect(act, SIGNAL(triggered()), SLOT(openSettings()));
    menu->addAction(act);

    menu = menuBar()->addMenu(tr("&Insert", "Main menu"));
    mainDock->addMenu(menu);

    act = new QAction(FbIcon("insert-image"), tr("&Image"), this);
    text->setAction(Fb::InsertImage, act);
    menu->addAction(act);

    act = new QAction(FbIcon("insert-text"), tr("&Footnote"), this);
    text->setAction(Fb::InsertNote, act);
    menu->addAction(act);

    act = new QAction(FbIcon("insert-link"), tr("&Hiperlink"), this);
    text->setAction(Fb::InsertLink, act);
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(tr("&Body"), this);
    text->setAction(Fb::InsertBody, act);
    menu->addAction(act);

    act = new QAction(FbIcon("insert-object"), tr("&Section"), this);
    text->setAction(Fb::InsertSection, act);
    menu->addAction(act);

    act = new QAction(tr("&Title"), this);
    text->setAction(Fb::InsertTitle, act);
    menu->addAction(act);

    act = new QAction(tr("&Epigraph"), this);
    text->setAction(Fb::InsertEpigraph, act);
    menu->addAction(act);

    act = new QAction(tr("&Annotation"), this);
    text->setAction(Fb::InsertAnnot, act);
    menu->addAction(act);

    act = new QAction(tr("&Subtitle"), this);
    text->setAction(Fb::InsertSubtitle, act);
    menu->addAction(act);

    act = new QAction(tr("&Cite"), this);
    text->setAction(Fb::InsertCite, act);
    menu->addAction(act);

    act = new QAction(tr("&Poem"), this);
    text->setAction(Fb::InsertPoem, act);
    menu->addAction(act);

    act = new QAction(tr("&Stanza"), this);
    text->setAction(Fb::InsertStanza, act);
    menu->addAction(act);

    act = new QAction(tr("&Author"), this);
    text->setAction(Fb::InsertAuthor, act);
    menu->addAction(act);

    act = new QAction(tr("&Date"), this);
    text->setAction(Fb::InsertDate, act);
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(tr("Simple text"), this);
    text->setAction(Fb::InsertText, act);
    menu->addAction(act);

    act = new FbTextAction(tr("Paragraph"), QWebPage::InsertParagraphSeparator, text);
    text->setAction(Fb::InsertParag, act);
    menu->addAction(act);

    menu = menuBar()->addMenu(tr("Fo&rmat"));
    mainDock->addMenu(menu);

    act = new FbTextAction(FbIcon("edit-clear"), tr("Clear format"), QWebPage::RemoveFormat, text);
    text->setAction(Fb::ClearFormat, act);
    menu->addAction(act);

    menu->addSeparator();

    act = new FbTextAction(FbIcon("format-text-bold"), tr("&Bold"), QWebPage::ToggleBold, text);
    text->setAction(Fb::TextBold, act);
    act->setShortcuts(QKeySequence::Bold);
    act->setCheckable(true);
    menu->addAction(act);

    act = new FbTextAction(FbIcon("format-text-italic"), tr("&Italic"), QWebPage::ToggleItalic, text);
    text->setAction(Fb::TextItalic, act);
    act->setShortcuts(QKeySequence::Italic);
    act->setCheckable(true);
    menu->addAction(act);

    act = new FbTextAction(FbIcon("format-text-strikethrough"), tr("&Strikethrough"), QWebPage::ToggleStrikethrough, text);
    text->setAction(Fb::TextStrike, act);
    act->setCheckable(true);
    menu->addAction(act);

    act = new FbTextAction(FbIcon("format-text-superscript"), tr("Su&perscript"), QWebPage::ToggleSuperscript, text);
    text->setAction(Fb::TextSup, act);
    act->setCheckable(true);
    menu->addAction(act);

    act = new FbTextAction(FbIcon("format-text-subscript"), tr("Su&bscript"), QWebPage::ToggleSubscript, text);
    text->setAction(Fb::TextSub, act);
    act->setCheckable(true);
    menu->addAction(act);

    act = new QAction(FbIcon("utilities-terminal"), tr("&Code"), this);
    text->setAction(Fb::TextCode, act);
    act->setCheckable(true);
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(FbIcon("format-indent-more"), tr("Create section"), text);
    text->setAction(Fb::SectionAdd, act);
    menu->addAction(act);

    act = new QAction(FbIcon("format-indent-less"), tr("Remove section"), text);
    text->setAction(Fb::SectionDel, act);
    menu->addAction(act);

    act = new QAction(FbIcon("format-justify-center"), tr("Make title"), text);
    text->setAction(Fb::TextTitle, act);
    menu->addAction(act);

    menu = menuBar()->addMenu(tr("&View"));

    tool->addSeparator();

    QActionGroup * viewGroup = new QActionGroup(this);

    act = new FbModeAction(mainDock, Fb::Text, FbIcon("x-office-document"), tr("&Text"));
    viewGroup->addAction(act);
    mainDock->addAction(Fb::Text, act);
    menu->addAction(act);
    tool->addAction(act);
    act->setChecked(true);

    act = new FbModeAction(mainDock, Fb::Head, FbIcon("document-properties"), tr("&Head"));
    viewGroup->addAction(act);
    mainDock->addAction(Fb::Head, act);
    menu->addAction(act);
    tool->addAction(act);

    act = new FbModeAction(mainDock, Fb::Code, FbIcon("text-x-generic"), tr("&XML"));
    viewGroup->addAction(act);
    mainDock->addAction(Fb::Code, act);
    menu->addAction(act);
    tool->addAction(act);

#ifdef QT_DEBUG
    act = new FbModeAction(mainDock, Fb::Html, FbIcon("text-html"), tr("&HTML"));
    mainDock->addAction(Fb::Html, act);
    viewGroup->addAction(act);
    menu->addAction(act);
#endif // QT_DEBUG

    menu->addSeparator();

    act = new QAction(FbIcon("zoom-in"), tr("Zoom in"), this);
    text->setAction(Fb::ZoomIn, act);
    code->setAction(Fb::ZoomIn, act);
    act->setShortcuts(QKeySequence::ZoomIn);
    menu->addAction(act);

    act = new QAction(FbIcon("zoom-out"), tr("Zoom out"), this);
    text->setAction(Fb::ZoomOut, act);
    code->setAction(Fb::ZoomOut, act);
    act->setShortcuts(QKeySequence::ZoomOut);
    menu->addAction(act);

    act = new QAction(FbIcon("zoom-original"), tr("Zoom original"), this);
    text->setAction(Fb::ZoomReset, act);
    code->setAction(Fb::ZoomReset, act);
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(tr("&Contents"), this);
    text->setAction(Fb::ViewContents, act);
    act->setCheckable(true);
    menu->addAction(act);

    act = new QAction(tr("&Pictures"), this);
    text->setAction(Fb::ViewPictures, act);
    act->setCheckable(true);
    menu->addAction(act);

    act = new QAction(tr("&Footnotes"), this);
    text->setAction(Fb::ViewFootnotes, act);
    act->setCheckable(true);
    menu->addAction(act);

    act = new QAction(tr("&Web inspector"), this);
    text->setAction(Fb::ViewInspector, act);
    act->setCheckable(true);
    menu->addAction(act);

    menuBar()->addSeparator();
    menu = menuBar()->addMenu(tr("&Help"));

    act = new QAction(FbIcon("help-about"), tr("&About"), this);
    act->setStatusTip(tr("Show the application's About box"));
    connect(act, SIGNAL(triggered()), this, SLOT(about()));
    menu->addAction(act);

    act = new QAction(tr("About &Qt"), this);
    act->setStatusTip(tr("Show the Qt library's About box"));
    connect(act, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    menu->addAction(act);

    toolEdit = tool = addToolBar(tr("Edit"));
    tool->setIconSize(QSize(24, 24));
    tool->setMovable(false);
    tool->addSeparator();
    mainDock->setTool(tool);
}

void FbMainWindow::openSettings()
{
    FbSetupDlg dlg(this);
    dlg.exec();
}

void FbMainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void FbMainWindow::readSettings()
{
    QSettings settings;
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    move(pos);
    resize(size);
}

void FbMainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

bool FbMainWindow::maybeSave()
{
    if (mainDock->isModified()) {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, qApp->applicationName(),
                     tr("The document has been modified. Do you want to save your changes?"),
                     QMessageBox::Save | QMessageBox::Discard
		     | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return fileSave();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

bool FbMainWindow::saveFile(const QString &fileName, const QString &codec)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, qApp->applicationName(), tr("Cannot write file %1: %2.").arg(fileName).arg(file.errorString()));
        return false;
    }
    bool ok = mainDock->save(&file, codec);
    setCurrentFile(fileName);
    return ok;
}

void FbMainWindow::setCurrentFile(const QString &filename)
{
    if (filename.isEmpty()) {
        static int sequenceNumber = 1;
        curFile = QString("book%1.fb2").arg(sequenceNumber++);
    } else {
        QFileInfo info = filename;
        curFile = info.canonicalFilePath();
    }
    setWindowFilePath(curFile);
    textChanged(false);
}

QString FbMainWindow::appTitle() const
{
    return QString(" - ") += qApp->applicationName() += QString(" ") += qApp->applicationVersion();
}

FbMainWindow *FbMainWindow::findFbMainWindow(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    for (QWidget *widget: qApp->topLevelWidgets()) {
        FbMainWindow *mainWin = qobject_cast<FbMainWindow *>(widget);
        if (mainWin && mainWin->curFile == canonicalFilePath)
            return mainWin;
    }
    return 0;
}
/*
void FbMainWindow::viewHead()
{
    if (headTree && centralWidget() == headTree) return;

    if (textFrame) textFrame->hideInspector();

    FbTextPage *page = 0;
    if (codeEdit) {
        QString xml = codeEdit->text();
        page = new FbTextPage(this);
        if (!page->load(QString(), xml)) {
            delete page;
            return;
        }
        isSwitched = true;
    }

    FB2DELETE(dockTree);
    FB2DELETE(dockImgs);
    FB2DELETE(codeEdit);
    FB2DELETE(toolEdit);

    if (!textFrame) {
        textFrame = new FbTextFrame(this, actionInspect);
        FbTextEdit *textEdit = textFrame->view();
        if (page) {
            page->setParent(textEdit);
            textEdit->setPage(page);
        }
    }

    if (!headTree) {
        headTree = new FbHeadEdit(this);
        headTree->setText(textFrame->view());
        connect(headTree, SIGNAL(status(QString)), this, SLOT(status(QString)));
    }

    this->setFocus();
    textFrame->setParent(NULL);
    setCentralWidget(headTree);
    textFrame->setParent(this);
    headTree->updateTree();

    headTree->setFocus();

    if (textFrame) {
        actionUndo->disconnect();
        actionRedo->disconnect();

        actionCut->disconnect();
        actionCopy->disconnect();
        actionPaste->disconnect();

        actionTextBold->disconnect();
        actionTextItalic->disconnect();
        actionTextStrike->disconnect();
        actionTextSub->disconnect();
        actionTextSup->disconnect();
    }

    FB2DELETE(toolEdit);
    toolEdit = addToolBar(tr("Edit"));
    headTree->initToolbar(*toolEdit);
    toolEdit->addSeparator();
    toolEdit->setMovable(false);

    actionContents->setEnabled(false);
    actionPictures->setEnabled(false);
    actionInspect->setEnabled(true);
}
*/
void FbMainWindow::status(const QString &text)
{
    statusBar()->showMessage(text);
}
