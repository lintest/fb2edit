#include <QtGui>
#include <QtDebug>
#include <QTreeView>
#include <QWebFrame>

#include "fb2main.hpp"
#include "fb2code.hpp"
#include "fb2read.hpp"
#include "fb2save.hpp"
#include "fb2tree.hpp"
#include "fb2view.hpp"
#include "fb2head.hpp"
#include "fb2utils.h"

Fb2MainWindow::Fb2MainWindow()
{
    init();
    setCurrentFile();
    viewText();
    textFrame->view.load(":blank.fb2");
}

Fb2MainWindow::Fb2MainWindow(const QString &filename, ViewMode mode)
{
    init();
    setCurrentFile(filename);
    if (mode == FB2) {
        viewText();
        textFrame->view.load(filename);
    } else {
        viewCode();
        loadXML(filename);
    }
}

void Fb2MainWindow::init()
{
    connect(qApp, SIGNAL(logMessage(QString)), SLOT(logMessage(QString)));

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(QIcon(":icon.ico"));

    isUntitled = true;

    createActions();
    createStatusBar();

    textFrame = NULL;
    noteEdit = NULL;
    codeEdit = NULL;
    headTree = NULL;
    toolEdit = NULL;
    dockTree = NULL;
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

void Fb2MainWindow::treeDestroyed()
{
    dockTree = NULL;
}

bool Fb2MainWindow::loadXML(const QString &filename)
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
    QString filename = QFileDialog::getOpenFileName(this, tr("Open file"), QString(), "Fiction book files (*.fb2)");
    if (filename.isEmpty()) return;

    Fb2MainWindow * existing = findFb2MainWindow(filename);
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
            Fb2MainWindow * other = new Fb2MainWindow(filename, FB2);
            other->move(x() + 40, y() + 40);
            other->show();
        }
    } else if (codeEdit) {
        if (isUntitled && !isWindowModified()) {
            setCurrentFile(filename);
            loadXML(filename);
        } else {
            Fb2MainWindow * other = new Fb2MainWindow(filename, XML);
            other->move(x() + 40, y() + 40);
            other->show();
        }
    }
}

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
    Fb2SaveDialog dlg(this, tr("Save As..."));
    dlg.selectFile(curFile);
    if (!dlg.exec()) return false;
    QString fileName = dlg.fileName();
    if (fileName.isEmpty()) return false;
    return saveFile(fileName, dlg.codec());
}

void Fb2MainWindow::about()
{
    QMessageBox::about(this, tr("About fb2edit"),
        tr("The <b>fb2edit</b> is application for editing FB2-files."));
}

void Fb2MainWindow::documentWasModified()
{
    bool modified = false;
    if (codeEdit) modified = codeEdit->isModified();
    QFileInfo info = windowFilePath();
    QString title = info.fileName();
    if (modified) title += QString("[*]");
    title += appTitle();
    setWindowTitle(title);
    setWindowModified(modified);
}

void Fb2MainWindow::cleanChanged(bool clean)
{
    QFileInfo info = windowFilePath();
    QString title = info.fileName();
    if (!clean) title += QString("[*]");
    title += appTitle();
    setWindowTitle(title);
    setWindowModified(!clean);
}

void Fb2MainWindow::createActions()
{
    QAction * act;
    QMenu * menu;
    QToolBar * tool;
    QList<QAction*> actions;

    menu = menuBar()->addMenu(tr("&File"));
    tool = addToolBar(tr("File"));
    tool->setMovable(false);

    act = new QAction(FB2::icon("document-new"), tr("&New"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::New);
    act->setStatusTip(tr("Create a new file"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileNew()));
    menu->addAction(act);
    tool->addAction(act);

    act = new QAction(FB2::icon("document-open"), tr("&Open..."), this);
    act->setShortcuts(QKeySequence::Open);
    act->setStatusTip(tr("Open an existing file"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileOpen()));
    menu->addAction(act);
    tool->addAction(act);

    act = new QAction(FB2::icon("document-save"), tr("&Save"), this);
    act->setShortcuts(QKeySequence::Save);
    act->setStatusTip(tr("Save the document to disk"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileSave()));
    menu->addAction(act);
    tool->addAction(act);

    act = new QAction(FB2::icon("document-save-as"), tr("Save &As..."), this);
    act->setShortcuts(QKeySequence::SaveAs);
    act->setStatusTip(tr("Save the document under a new name"));
    connect(act, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(FB2::icon("window-close"), tr("&Close"), this);
    act->setShortcuts(QKeySequence::Close);
    act->setStatusTip(tr("Close this window"));
    connect(act, SIGNAL(triggered()), this, SLOT(close()));
    menu->addAction(act);

    act = new QAction(FB2::icon("application-exit"), tr("E&xit"), this);
    act->setShortcuts(QKeySequence::Quit);
    act->setStatusTip(tr("Exit the application"));
    connect(act, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
    menu->addAction(act);

    menuEdit = menu = menuBar()->addMenu(tr("&Edit"));

    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));

    actionUndo = act = new QAction(FB2::icon("edit-undo"), tr("&Undo"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Undo);
    act->setEnabled(false);
    menu->addAction(act);

    actionRedo = act = new QAction(FB2::icon("edit-redo"), tr("&Redo"), this);
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Redo);
    act->setEnabled(false);
    menu->addAction(act);

    menu->addSeparator();

    actionCut = act = new QAction(FB2::icon("edit-cut"), tr("Cu&t"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Cut);
    act->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    act->setEnabled(false);
    menu->addAction(act);

    actionCopy = act = new QAction(FB2::icon("edit-copy"), tr("&Copy"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Copy);
    act->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    act->setEnabled(false);
    menu->addAction(act);

    actionPaste = act = new QAction(FB2::icon("edit-paste"), tr("&Paste"), this);
    act->setShortcutContext(Qt::WidgetShortcut);
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Paste);
    act->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    menu->addAction(act);
    clipboardDataChanged();

    menu->addSeparator();

    actionFind = act = new QAction(FB2::icon("edit-find"), tr("&Find..."), this);
    act->setShortcuts(QKeySequence::Find);
    menu->addAction(act);

    actionReplace = act = new QAction(FB2::icon("edit-find-replace"), tr("&Replace..."), this);
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(FB2::icon("preferences-desktop"), tr("&Settings"), this);
    act->setShortcuts(QKeySequence::Preferences);
    act->setStatusTip(tr("Application settings"));
    connect(act, SIGNAL(triggered()), SLOT(openSettings()));
    menu->addAction(act);

    menu = menuBar()->addMenu(tr("&Insert", "Main menu"));

    actionImage = act = new QAction(FB2::icon("insert-image"), tr("&Image"), this);
    menu->addAction(act);

    actionNote = act = new QAction(FB2::icon("insert-text"), tr("&Footnote"), this);
    menu->addAction(act);

    actionLink = act = new QAction(FB2::icon("insert-link"), tr("&Hiperlink"), this);
    menu->addAction(act);

    menu->addSeparator();

    actionSection = act = new QAction(FB2::icon("insert-object"), tr("&Section"), this);
    menu->addAction(act);

    actionTitle = act = new QAction(tr("&Title"), this);
    menu->addAction(act);

    actionSubtitle = act = new QAction(tr("&Subtitle"), this);
    menu->addAction(act);

    actionAuthor = act = new QAction(tr("&Author"), this);
    menu->addAction(act);

    actionDescr = act = new QAction(tr("&Annotation"), this);
    menu->addAction(act);

    actionPoem = act = new QAction(tr("&Poem"), this);
    menu->addAction(act);

    actionStanza = act = new QAction(tr("&Stanza"), this);
    menu->addAction(act);

    actionBody = act = new QAction(tr("&Body"), this);
    menu->addAction(act);

    menuText = menu = menuBar()->addMenu(tr("Fo&rmat"));

    actionTextBold = act = new QAction(FB2::icon("format-text-bold"), tr("&Bold"), this);
    act->setShortcuts(QKeySequence::Bold);
    act->setCheckable(true);
    menu->addAction(act);

    actionTextItalic = act = new QAction(FB2::icon("format-text-italic"), tr("&Italic"), this);
    act->setShortcuts(QKeySequence::Italic);
    act->setCheckable(true);
    menu->addAction(act);

    actionTextStrike = act = new QAction(FB2::icon("format-text-strikethrough"), tr("&Strikethrough"), this);
    act->setCheckable(true);
    menu->addAction(act);

    actionTextSup = act = new QAction(FB2::icon("format-text-superscript"), tr("Su&perscript"), this);
    act->setCheckable(true);
    menu->addAction(act);

    actionTextSub = act = new QAction(FB2::icon("format-text-subscript"), tr("Su&bscript"), this);
    act->setCheckable(true);
    menu->addAction(act);

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

    actionZoomIn = act = new QAction(FB2::icon("zoom-in"), tr("Zoom in"), this);
    act->setShortcuts(QKeySequence::ZoomIn);
    menu->addAction(act);

    actionZoomOut = act = new QAction(FB2::icon("zoom-out"), tr("Zoom out"), this);
    act->setShortcuts(QKeySequence::ZoomOut);
    menu->addAction(act);

    actionZoomReset = act = new QAction(FB2::icon("zoom-original"), tr("Zoom original"), this);
    menu->addAction(act);

    menu->addSeparator();

    act = new QAction(tr("&Contents"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(viewTree()));
    menu->addAction(act);

    actionInspect = act = new QAction(tr("&Web inspector"), this);
    menu->addAction(act);

    menuBar()->addSeparator();
    menu = menuBar()->addMenu(tr("&Help"));

    act = new QAction(FB2::icon("help-about"), tr("&About"), this);
    act->setStatusTip(tr("Show the application's About box"));
    connect(act, SIGNAL(triggered()), this, SLOT(about()));
    menu->addAction(act);

    act = new QAction(tr("About &Qt"), this);
    act->setStatusTip(tr("Show the Qt library's About box"));
    connect(act, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    menu->addAction(act);
}

void Fb2MainWindow::openSettings()
{
    QMessageBox::about(this, tr("Settings"),
        tr("The <b>fb2edit</b> is application for editing FB2-files."));
}

void Fb2MainWindow::createTree()
{
    if (textFrame && centralWidget() == textFrame) {
        dockTree = new QDockWidget(tr("Contents"), this);
        dockTree->setAttribute(Qt::WA_DeleteOnClose);
        dockTree->setFeatures(QDockWidget::AllDockWidgetFeatures);
        dockTree->setWidget(new Fb2TreeWidget(textFrame->view, this));
        connect(dockTree, SIGNAL(destroyed()), SLOT(treeDestroyed()));
        addDockWidget(Qt::LeftDockWidgetArea, dockTree);
    }
}

void Fb2MainWindow::selectionChanged()
{
    actionCut->setEnabled(textFrame->view.CutEnabled());
    actionCopy->setEnabled(textFrame->view.CopyEnabled());

    actionTextBold->setChecked(textFrame->view.BoldChecked());
    actionTextItalic->setChecked(textFrame->view.ItalicChecked());
    actionTextStrike->setChecked(textFrame->view.StrikeChecked());
    actionTextSub->setChecked(textFrame->view.SubChecked());
    actionTextSup->setChecked(textFrame->view.SupChecked());

    statusBar()->showMessage(textFrame->view.status());
}

void Fb2MainWindow::canUndoChanged(bool canUndo)
{
    actionUndo->setEnabled(canUndo);
}

void Fb2MainWindow::canRedoChanged(bool canRedo)
{
    actionRedo->setEnabled(canRedo);
}

void Fb2MainWindow::undoChanged()
{
    actionUndo->setEnabled(textFrame->view.UndoEnabled());
}

void Fb2MainWindow::redoChanged()
{
    actionRedo->setEnabled(textFrame->view.RedoEnabled());
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

bool Fb2MainWindow::saveFile(const QString &fileName, const QString &codec)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, qApp->applicationName(), tr("Cannot write file %1: %2.").arg(fileName).arg(file.errorString()));
        return false;
    }

    if (textFrame) {
        textFrame->view.save(&file, codec);
        setCurrentFile(fileName);
    }
    return true;

/*
    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << textFrame->view.toPlainText();
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
*/
}

void Fb2MainWindow::setCurrentFile(const QString &filename)
{
    static int sequenceNumber = 1;

    QString title;
    isUntitled = filename.isEmpty();
    if (isUntitled) {
        curFile = QString("book%1.fb2").arg(sequenceNumber++);
        title = curFile;
    } else {
        QFileInfo info = filename;
        curFile = info.canonicalFilePath();
        title = info.fileName();
    }
    title += appTitle();

    setWindowModified(false);
    setWindowFilePath(curFile);
    setWindowTitle(title);
}

QString Fb2MainWindow::appTitle() const
{
    return QString(" - ") += qApp->applicationName() += QString(" ") += qApp->applicationVersion();
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

void Fb2MainWindow::checkScintillaUndo()
{
    if (!codeEdit) return;
    actionUndo->setEnabled(codeEdit->isUndoAvailable());
    actionRedo->setEnabled(codeEdit->isRedoAvailable());
}

void Fb2MainWindow::viewCode()
{
    if (codeEdit && centralWidget() == codeEdit) return;

    bool load = false;
    QByteArray xml;
    QList<int> folds;
    if (textFrame) {
        textFrame->view.save(&xml);
        load = true;
    }

    FB2DELETE(textFrame);
    FB2DELETE(dockTree);
    FB2DELETE(headTree);

    if (!codeEdit) {
        codeEdit = new Fb2CodeEdit;
    }
    if (load) codeEdit->load(xml, folds);
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

void Fb2MainWindow::viewText()
{
    if (textFrame && centralWidget() == textFrame) return;
    QString xml;
    if (codeEdit) xml = codeEdit->text();
    FB2DELETE(codeEdit);
    FB2DELETE(headTree);
    if (!textFrame) {
        textFrame = new Fb2WebFrame(this);
    }
    setCentralWidget(textFrame);
    textFrame->view.setFocus();
    viewTree();

    connect(textFrame->view.page()->undoStack(), SIGNAL(cleanChanged(bool)), SLOT(cleanChanged(bool)));
    connect(textFrame->view.page()->undoStack(), SIGNAL(canUndoChanged(bool)), SLOT(canUndoChanged(bool)));
    connect(textFrame->view.page()->undoStack(), SIGNAL(canRedoChanged(bool)), SLOT(canRedoChanged(bool)));
    connect(textFrame->view.page(), SIGNAL(selectionChanged()), SLOT(selectionChanged()));

    connect(textFrame->view.pageAction(QWebPage::Undo), SIGNAL(changed()), SLOT(undoChanged()));
    connect(textFrame->view.pageAction(QWebPage::Redo), SIGNAL(changed()), SLOT(redoChanged()));
    connect(actionUndo, SIGNAL(triggered()), textFrame->view.pageAction(QWebPage::Undo), SIGNAL(triggered()));
    connect(actionRedo, SIGNAL(triggered()), textFrame->view.pageAction(QWebPage::Redo), SIGNAL(triggered()));

    connect(actionCut, SIGNAL(triggered()), textFrame->view.pageAction(QWebPage::Cut), SIGNAL(triggered()));
    connect(actionCopy, SIGNAL(triggered()), textFrame->view.pageAction(QWebPage::Copy), SIGNAL(triggered()));
    connect(actionPaste, SIGNAL(triggered()), textFrame->view.pageAction(QWebPage::Paste), SIGNAL(triggered()));

    connect(actionTextBold, SIGNAL(triggered()), textFrame->view.pageAction(QWebPage::ToggleBold), SIGNAL(triggered()));
    connect(actionTextItalic, SIGNAL(triggered()), textFrame->view.pageAction(QWebPage::ToggleItalic), SIGNAL(triggered()));
    connect(actionTextStrike, SIGNAL(triggered()), textFrame->view.pageAction(QWebPage::ToggleStrikethrough), SIGNAL(triggered()));
    connect(actionTextSub, SIGNAL(triggered()), textFrame->view.pageAction(QWebPage::ToggleSubscript), SIGNAL(triggered()));
    connect(actionTextSup, SIGNAL(triggered()), textFrame->view.pageAction(QWebPage::ToggleSuperscript), SIGNAL(triggered()));

    QWebView * textEdit = &(textFrame->view);

    connect(actionFind, SIGNAL(triggered()), textEdit, SLOT(find()));
    connect(actionImage, SIGNAL(triggered()), textEdit, SLOT(insertImage()));
    connect(actionNote, SIGNAL(triggered()), textEdit, SLOT(insertNote()));
    connect(actionLink, SIGNAL(triggered()), textEdit, SLOT(insertLink()));
    connect(actionTitle, SIGNAL(triggered()), textEdit, SLOT(insertTitle()));
    connect(actionBody, SIGNAL(triggered()), textEdit->page(), SLOT(insertBody()));

    connect(actionZoomIn, SIGNAL(triggered()), textEdit, SLOT(zoomIn()));
    connect(actionZoomOut, SIGNAL(triggered()), textEdit, SLOT(zoomOut()));
    connect(actionZoomReset, SIGNAL(triggered()), textEdit, SLOT(zoomReset()));
    connect(actionInspect, SIGNAL(triggered()), textFrame, SLOT(showInspector()));

    if (!xml.isEmpty()) textFrame->view.load(curFile, xml);

    FB2DELETE(toolEdit);
    QToolBar *tool = toolEdit = addToolBar(tr("Edit"));
    tool->setMovable(false);
    tool->addSeparator();

    FB2::addTools(tool, textEdit);

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

void Fb2MainWindow::viewHead()
{
    if (headTree && centralWidget() == headTree) return;

    if (textFrame) textFrame->hideInspector();

    QString xml;
    if (codeEdit) xml = codeEdit->text();

    FB2DELETE(dockTree);
    FB2DELETE(codeEdit);
    FB2DELETE(toolEdit);

    if (!textFrame) {
        textFrame = new Fb2WebFrame(this);
    }

    if (!headTree) {
        headTree = new Fb2HeadView(textFrame->view, this);
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

void Fb2MainWindow::viewTree()
{
    if (dockTree) dockTree->deleteLater(); else createTree();
}

void Fb2MainWindow::clipboardDataChanged()
{
    if (const QMimeData *md = QApplication::clipboard()->mimeData()) {
        actionPaste->setEnabled(md->hasText());
    }
}

void Fb2MainWindow::status(const QString &text)
{
    statusBar()->showMessage(text);
}
