#include <QtGui>
#include <QtDebug>
#include <QTreeView>
#include <QWebFrame>

#include "fb2main.hpp"
#include "fb2code.hpp"
#include "fb2read.hpp"
#include "fb2save.hpp"
#include "fb2text.hpp"
#include "fb2tree.hpp"
#include "fb2head.hpp"
#include "fb2utils.h"

//---------------------------------------------------------------------------
//  FbDockWidget
//---------------------------------------------------------------------------

FbDockWidget::FbDockWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(title, parent, flags)
{
    setFeatures(QDockWidget::AllDockWidgetFeatures);
    setAttribute(Qt::WA_DeleteOnClose);
}

//---------------------------------------------------------------------------
//  FbMainWindow
//---------------------------------------------------------------------------

FbMainWindow::FbMainWindow(const QString &filename, ViewMode mode)
    : QMainWindow()
    , textFrame(0)
    , codeEdit(0)
    , headTree(0)
    , noteEdit(0)
    , toolEdit(0)
    , dockTree(0)
    , dockImgs(0)
    , inspector(0)
    , messageEdit(0)
    , isSwitched(false)
    , isUntitled(true)
{
    connect(qApp, SIGNAL(logMessage(QString)), SLOT(logMessage(QString)));

    setUnifiedTitleAndToolBarOnMac(true);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(QIcon(":icon.ico"));

    createActions();
    createStatusBar();
    readSettings();

    setCurrentFile(filename);
    if (mode == FB2) {
        viewText();
        textFrame->view.load(filename.isEmpty() ? ":blank.fb2" : filename);
    } else {
        viewCode();
        if (!filename.isEmpty()) loadXML(filename);
    }
}

void FbMainWindow::logMessage(const QString &message)
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

void FbMainWindow::logShowed()
{
    messageEdit->setMaximumHeight(QWIDGETSIZE_MAX);
}

void FbMainWindow::logDestroyed()
{
    messageEdit = NULL;
}

void FbMainWindow::treeDestroyed()
{
    dockTree = NULL;
}

void FbMainWindow::imgsDestroyed()
{
    dockImgs = NULL;
}

bool FbMainWindow::loadXML(const QString &filename)
{
    if (!filename.isEmpty()) {
        QFile file(filename);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            codeEdit->clear();
            return codeEdit->read(&file);
        }
    }
    return false;
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
    if (filename.isEmpty()) return;

    FbMainWindow * existing = findFbMainWindow(filename);
    if (existing) {
        existing->show();
        existing->raise();
        existing->activateWindow();
        return;
    }

    if (textFrame) {
        if (isUntitled && !isWindowModified()) {
            setCurrentFile(filename);
            textFrame->view.load(filename);
        } else {
            FbMainWindow * other = new FbMainWindow(filename, FB2);
            other->move(x() + 40, y() + 40);
            other->show();
        }
    } else if (codeEdit) {
        if (isUntitled && !isWindowModified()) {
            setCurrentFile(filename);
            loadXML(filename);
        } else {
            FbMainWindow * other = new FbMainWindow(filename, XML);
            other->move(x() + 40, y() + 40);
            other->show();
        }
    }
}

bool FbMainWindow::fileSave()
{
    if (isUntitled) {
        return fileSaveAs();
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
    QMessageBox::about(this, tr("About fb2edit"),
        tr("The <b>fb2edit</b> is application for editing FB2-files."));
}

void FbMainWindow::documentWasModified()
{
    setModified(isSwitched || codeEdit->isModified());
}

void FbMainWindow::cleanChanged(bool clean)
{
    setModified(isSwitched || !clean);
}

void FbMainWindow::setModified(bool modified)
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
    QList<QAction*> actions;

    menu = menuBar()->addMenu(tr("&File"));
    tool = addToolBar(tr("File"));
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

    menuEdit = menu = menuBar()->addMenu(tr("&Edit"));

    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));

    actionUndo = act = new QAction(FbIcon("edit-undo"), tr("&Undo"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Undo);
    act->setEnabled(false);
    menu->addAction(act);

    actionRedo = act = new QAction(FbIcon("edit-redo"), tr("&Redo"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Redo);
    act->setEnabled(false);
    menu->addAction(act);

    menu->addSeparator();

    actionCut = act = new QAction(FbIcon("edit-cut"), tr("Cu&t"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Cut);
    act->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    act->setEnabled(false);
    menu->addAction(act);

    actionCopy = act = new QAction(FbIcon("edit-copy"), tr("&Copy"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Copy);
    act->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    act->setEnabled(false);
    menu->addAction(act);

    actionPaste = act = new QAction(FbIcon("edit-paste"), tr("&Paste"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Paste);
    act->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    menu->addAction(act);
    clipboardDataChanged();

    menu->addSeparator();

    actionFind = act = new QAction(FbIcon("edit-find"), tr("&Find..."), this);
    act->setShortcuts(QKeySequence::Find);
    menu->addAction(act);

    actionReplace = act = new QAction(FbIcon("edit-find-replace"), tr("&Replace..."), this);
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(FbIcon("preferences-desktop"), tr("&Settings"), this);
    act->setShortcuts(QKeySequence::Preferences);
    act->setStatusTip(tr("Application settings"));
    connect(act, SIGNAL(triggered()), SLOT(openSettings()));
    menu->addAction(act);

    menu = menuBar()->addMenu(tr("&Insert", "Main menu"));

    actionImage = act = new QAction(FbIcon("insert-image"), tr("&Image"), this);
    menu->addAction(act);

    actionNote = act = new QAction(FbIcon("insert-text"), tr("&Footnote"), this);
    menu->addAction(act);

    actionLink = act = new QAction(FbIcon("insert-link"), tr("&Hiperlink"), this);
    menu->addAction(act);

    menu->addSeparator();

    actionBody = act = new QAction(tr("&Body"), this);
    menu->addAction(act);

    actionSection = act = new QAction(FbIcon("insert-object"), tr("&Section"), this);
    menu->addAction(act);

    actionTitle = act = new QAction(tr("&Title"), this);
    menu->addAction(act);

    actionEpigraph = act = new QAction(tr("&Epigraph"), this);
    menu->addAction(act);

    actionAnnot = act = new QAction(tr("&Annotation"), this);
    menu->addAction(act);

    actionSubtitle = act = new QAction(tr("&Subtitle"), this);
    menu->addAction(act);

    actionAuthor = act = new QAction(tr("&Cite"), this);
    menu->addAction(act);

    actionPoem = act = new QAction(tr("&Poem"), this);
    menu->addAction(act);

    actionStanza = act = new QAction(tr("&Stanza"), this);
    menu->addAction(act);

    actionAuthor = act = new QAction(tr("&Author"), this);
    menu->addAction(act);

    actionDate = act = new QAction(tr("&Date"), this);
    menu->addAction(act);

    menuText = menu = menuBar()->addMenu(tr("Fo&rmat"));

    actionTextBold = act = new QAction(FbIcon("format-text-bold"), tr("&Bold"), this);
    act->setShortcuts(QKeySequence::Bold);
    act->setCheckable(true);
    menu->addAction(act);

    actionTextItalic = act = new QAction(FbIcon("format-text-italic"), tr("&Italic"), this);
    act->setShortcuts(QKeySequence::Italic);
    act->setCheckable(true);
    menu->addAction(act);

    actionTextStrike = act = new QAction(FbIcon("format-text-strikethrough"), tr("&Strikethrough"), this);
    act->setCheckable(true);
    menu->addAction(act);

    actionTextSup = act = new QAction(FbIcon("format-text-superscript"), tr("Su&perscript"), this);
    act->setCheckable(true);
    menu->addAction(act);

    actionTextSub = act = new QAction(FbIcon("format-text-subscript"), tr("Su&bscript"), this);
    act->setCheckable(true);
    menu->addAction(act);

    actionTextCode = act = new QAction(tr("&Code"), this);
    act->setCheckable(true);
    menu->addAction(act);
    act->setEnabled(false);

    menuView = menu = menuBar()->addMenu(tr("&View"));

    tool->addSeparator();

    QActionGroup * viewGroup = new QActionGroup(this);

    act = new QAction(tr("&Text"), this);
    act->setCheckable(true);
    act->setChecked(true);
    connect(act, SIGNAL(triggered()), this, SLOT(viewText()));
    viewGroup->addAction(act);
    menu->addAction(act);
    tool->addAction(act);

    act = new QAction(tr("&Head"), this);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), this, SLOT(viewHead()));
    viewGroup->addAction(act);
    menu->addAction(act);
    tool->addAction(act);

    act = new QAction(tr("&XML"), this);
    act->setCheckable(true);
    connect(act, SIGNAL(triggered()), this, SLOT(viewCode()));
    viewGroup->addAction(act);
    menu->addAction(act);
    tool->addAction(act);

    menu->addSeparator();

    actionZoomIn = act = new QAction(FbIcon("zoom-in"), tr("Zoom in"), this);
    act->setShortcuts(QKeySequence::ZoomIn);
    menu->addAction(act);

    actionZoomOut = act = new QAction(FbIcon("zoom-out"), tr("Zoom out"), this);
    act->setShortcuts(QKeySequence::ZoomOut);
    menu->addAction(act);

    actionZoomReset = act = new QAction(FbIcon("zoom-original"), tr("Zoom original"), this);
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(tr("&Contents"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(viewTree()));
    menu->addAction(act);

    act = new QAction(tr("&Pictures"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(viewImgs()));
    menu->addAction(act);

    actionInspect = act = new QAction(tr("&Web inspector"), this);
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
}

void FbMainWindow::openSettings()
{
    QMessageBox::about(this, tr("Settings"),
        tr("The <b>fb2edit</b> is application for editing FB2-files."));
}

void FbMainWindow::createTree()
{
    if (textFrame && centralWidget() == textFrame) {
        dockTree = new FbDockWidget(tr("Contents"), this);
        dockTree->setWidget(new FbTreeWidget(textFrame->view, this));
        connect(dockTree, SIGNAL(destroyed()), SLOT(treeDestroyed()));
        addDockWidget(Qt::LeftDockWidgetArea, dockTree);
    }
}

void FbMainWindow::createImgs()
{
    if (textFrame && centralWidget() == textFrame) {
        dockImgs = new FbDockWidget(tr("Pictures"), this);
        dockImgs->setWidget(new FbListWidget(textFrame->view, this));
        connect(dockImgs, SIGNAL(destroyed()), SLOT(imgsDestroyed()));
        addDockWidget(Qt::RightDockWidgetArea, dockImgs);
    }
}

void FbMainWindow::selectionChanged()
{
    actionCut->setEnabled(textFrame->view.CutEnabled());
    actionCopy->setEnabled(textFrame->view.CopyEnabled());

    actionTextBold->setChecked(textFrame->view.BoldChecked());
    actionTextItalic->setChecked(textFrame->view.ItalicChecked());
    actionTextStrike->setChecked(textFrame->view.StrikeChecked());
    actionTextSub->setChecked(textFrame->view.SubChecked());
    actionTextSup->setChecked(textFrame->view.SupChecked());

    statusBar()->showMessage(textFrame->view.page()->status());
}

void FbMainWindow::canUndoChanged(bool canUndo)
{
    actionUndo->setEnabled(canUndo);
}

void FbMainWindow::canRedoChanged(bool canRedo)
{
    actionRedo->setEnabled(canRedo);
}

void FbMainWindow::undoChanged()
{
    actionUndo->setEnabled(textFrame->view.UndoEnabled());
}

void FbMainWindow::redoChanged()
{
    actionRedo->setEnabled(textFrame->view.RedoEnabled());
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
    if (textFrame && textFrame->view.isModified()) {
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

    if (textFrame) {
        isSwitched = false;
        textFrame->view.save(&file, codec);
        setCurrentFile(fileName);
        return true;
    }

    if (codeEdit) {
        QTextStream out(&file);
        out << codeEdit->toPlainText();
        setCurrentFile(fileName);
        return true;
    }

    return false;
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
    setModified(false);
}

QString FbMainWindow::appTitle() const
{
    return QString(" - ") += qApp->applicationName() += QString(" ") += qApp->applicationVersion();
}

FbMainWindow *FbMainWindow::findFbMainWindow(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QWidget *widget, qApp->topLevelWidgets()) {
        FbMainWindow *mainWin = qobject_cast<FbMainWindow *>(widget);
        if (mainWin && mainWin->curFile == canonicalFilePath)
            return mainWin;
    }
    return 0;
}

void FbMainWindow::checkScintillaUndo()
{
    if (!codeEdit) return;
    actionUndo->setEnabled(codeEdit->isUndoAvailable());
    actionRedo->setEnabled(codeEdit->isRedoAvailable());
}

void FbMainWindow::viewCode()
{
    if (codeEdit && centralWidget() == codeEdit) return;

    bool load = false;
    QByteArray xml;
    if (textFrame) {
        textFrame->view.save(&xml);
        isSwitched = true;
        load = true;
    }

    FB2DELETE(textFrame);
    FB2DELETE(dockTree);
    FB2DELETE(headTree);

    if (!codeEdit) {
        codeEdit = new FbCodeEdit;
    }
    if (load) codeEdit->load(xml);
    setCentralWidget(codeEdit);
    codeEdit->setFocus();

    FB2DELETE(toolEdit);
    QToolBar *tool = toolEdit = addToolBar(tr("Edit"));
    tool->addSeparator();
    tool->addAction(actionUndo);
    tool->addAction(actionRedo);
    tool->addSeparator();
    tool->addAction(actionCut);
    tool->addAction(actionCopy);
    tool->addAction(actionPaste);
    tool->addSeparator();
    tool->addAction(actionZoomIn);
    tool->addAction(actionZoomOut);
    tool->addAction(actionZoomReset);
    tool->setMovable(false);

    connect(codeEdit, SIGNAL(textChanged()), this, SLOT(documentWasModified()));
    connect(codeEdit, SIGNAL(textChanged()), this, SLOT(checkScintillaUndo()));

    connect(codeEdit, SIGNAL(copyAvailable(bool)), actionCut, SLOT(setEnabled(bool)));
    connect(codeEdit, SIGNAL(copyAvailable(bool)), actionCopy, SLOT(setEnabled(bool)));

    connect(actionUndo, SIGNAL(triggered()), codeEdit, SLOT(undo()));
    connect(actionRedo, SIGNAL(triggered()), codeEdit, SLOT(redo()));

    connect(actionCut, SIGNAL(triggered()), codeEdit, SLOT(cut()));
    connect(actionCopy, SIGNAL(triggered()), codeEdit, SLOT(copy()));
    connect(actionPaste, SIGNAL(triggered()), codeEdit, SLOT(paste()));

    connect(actionFind, SIGNAL(triggered()), codeEdit, SLOT(find()));

    connect(actionZoomIn, SIGNAL(triggered()), codeEdit, SLOT(zoomIn()));
    connect(actionZoomOut, SIGNAL(triggered()), codeEdit, SLOT(zoomOut()));
    connect(actionZoomReset, SIGNAL(triggered()), codeEdit, SLOT(zoomReset()));
}

void FbMainWindow::viewText()
{
    if (textFrame && centralWidget() == textFrame) return;

    bool load = false;
    QString xml;
    if (codeEdit) {
        xml = codeEdit->text();
        isSwitched = true;
        load = true;
    }

    FB2DELETE(codeEdit);
    FB2DELETE(headTree);
    if (!textFrame) {
        textFrame = new FbTextFrame(this);
    }
    setCentralWidget(textFrame);
    textFrame->view.setFocus();
    viewTree();

    FbTextEdit * textEdit = &textFrame->view;
    FbTextPage * textPage = textEdit->page();

    connect(textPage->undoStack(), SIGNAL(cleanChanged(bool)), SLOT(cleanChanged(bool)));
    connect(textPage->undoStack(), SIGNAL(canUndoChanged(bool)), SLOT(canUndoChanged(bool)));
    connect(textPage->undoStack(), SIGNAL(canRedoChanged(bool)), SLOT(canRedoChanged(bool)));
    connect(textPage, SIGNAL(selectionChanged()), SLOT(selectionChanged()));

    connect(textEdit->pageAction(QWebPage::Undo), SIGNAL(changed()), SLOT(undoChanged()));
    connect(textEdit->pageAction(QWebPage::Redo), SIGNAL(changed()), SLOT(redoChanged()));
    connect(actionUndo, SIGNAL(triggered()), textEdit->pageAction(QWebPage::Undo), SIGNAL(triggered()));
    connect(actionRedo, SIGNAL(triggered()), textEdit->pageAction(QWebPage::Redo), SIGNAL(triggered()));

    connect(actionCut, SIGNAL(triggered()), textEdit->pageAction(QWebPage::Cut), SIGNAL(triggered()));
    connect(actionCopy, SIGNAL(triggered()), textEdit->pageAction(QWebPage::Copy), SIGNAL(triggered()));
    connect(actionPaste, SIGNAL(triggered()), textEdit->pageAction(QWebPage::Paste), SIGNAL(triggered()));

    connect(actionTextBold, SIGNAL(triggered()), textEdit->pageAction(QWebPage::ToggleBold), SIGNAL(triggered()));
    connect(actionTextItalic, SIGNAL(triggered()), textEdit->pageAction(QWebPage::ToggleItalic), SIGNAL(triggered()));
    connect(actionTextStrike, SIGNAL(triggered()), textEdit->pageAction(QWebPage::ToggleStrikethrough), SIGNAL(triggered()));
    connect(actionTextSub, SIGNAL(triggered()), textEdit->pageAction(QWebPage::ToggleSubscript), SIGNAL(triggered()));
    connect(actionTextSup, SIGNAL(triggered()), textEdit->pageAction(QWebPage::ToggleSuperscript), SIGNAL(triggered()));

    connect(actionFind, SIGNAL(triggered()), textEdit, SLOT(find()));
    connect(actionImage, SIGNAL(triggered()), textEdit, SLOT(insertImage()));
    connect(actionNote, SIGNAL(triggered()), textEdit, SLOT(insertNote()));
    connect(actionLink, SIGNAL(triggered()), textEdit, SLOT(insertLink()));

    connect(actionTitle, SIGNAL(triggered()), textPage, SLOT(insertTitle()));
    connect(actionAnnot, SIGNAL(triggered()), textPage, SLOT(insertAnnot()));
    connect(actionAuthor, SIGNAL(triggered()), textPage, SLOT(insertAuthor()));
    connect(actionEpigraph, SIGNAL(triggered()), textPage, SLOT(insertEpigraph()));
    connect(actionSubtitle, SIGNAL(triggered()), textPage, SLOT(insertSubtitle()));
    connect(actionSection, SIGNAL(triggered()), textPage, SLOT(insertSection()));
    connect(actionStanza, SIGNAL(triggered()), textPage, SLOT(insertStanza()));
    connect(actionPoem, SIGNAL(triggered()), textPage, SLOT(insertPoem()));
    connect(actionDate, SIGNAL(triggered()), textPage, SLOT(insertDate()));
    connect(actionBody, SIGNAL(triggered()), textPage, SLOT(insertBody()));

    connect(actionZoomIn, SIGNAL(triggered()), textEdit, SLOT(zoomIn()));
    connect(actionZoomOut, SIGNAL(triggered()), textEdit, SLOT(zoomOut()));
    connect(actionZoomReset, SIGNAL(triggered()), textEdit, SLOT(zoomReset()));
    connect(actionInspect, SIGNAL(triggered()), textFrame, SLOT(showInspector()));

    if (load) textFrame->view.load(curFile, xml);

    FB2DELETE(toolEdit);
    QToolBar *tool = toolEdit = addToolBar(tr("Edit"));
    tool->setMovable(false);
    tool->addSeparator();

    textEdit->addTools(tool);

    tool->addSeparator();

    tool->addAction(actionImage);
    tool->addAction(actionNote);
    tool->addAction(actionLink);
    tool->addAction(actionSection);

    tool->addSeparator();

    tool->addAction(actionZoomIn);
    tool->addAction(actionZoomOut);
    tool->addAction(actionZoomReset);
}

void FbMainWindow::viewHead()
{
    if (headTree && centralWidget() == headTree) return;

    if (textFrame) textFrame->hideInspector();

    QString xml;
    if (codeEdit) xml = codeEdit->text();

    FB2DELETE(dockTree);
    FB2DELETE(codeEdit);
    FB2DELETE(toolEdit);

    if (!textFrame) {
        textFrame = new FbTextFrame(this);
    }

    if (!headTree) {
        headTree = new FbHeadView(textFrame->view, this);
        connect(headTree, SIGNAL(status(QString)), this, SLOT(status(QString)));
    }

    this->setFocus();
    textFrame->setParent(NULL);
    setCentralWidget(headTree);
    textFrame->setParent(this);
    headTree->updateTree();

    headTree->setFocus();

    if (!xml.isEmpty()) textFrame->view.load(curFile, xml);

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
}

void FbMainWindow::viewTree()
{
    if (dockTree) dockTree->deleteLater(); else createTree();
}

void FbMainWindow::viewImgs()
{
    if (dockImgs) dockImgs->deleteLater(); else createImgs();
}

void FbMainWindow::clipboardDataChanged()
{
    if (const QMimeData *md = QApplication::clipboard()->mimeData()) {
        actionPaste->setEnabled(md->hasText());
    }
}

void FbMainWindow::status(const QString &text)
{
    statusBar()->showMessage(text);
}
