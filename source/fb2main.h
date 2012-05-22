#ifndef FB2MAIN_H
#define FB2MAIN_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QDockWidget;
class QFile;
class QMenu;
class QModelIndex;
class QTextEdit;
class QTreeView;
class QWebInspector;
QT_END_NAMESPACE

class QsciScintilla;
class Fb2MainDocument;
class Fb2WebView;

class Fb2MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum ViewMode { FB2, XML };
    explicit Fb2MainWindow();
    explicit Fb2MainWindow(const QString &filename, ViewMode mode = FB2);

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    void logMessage(const QString &message);

private slots:
    void fileNew();
    void fileOpen();
    bool fileSave();
    bool fileSaveAs();

    void about();
    void documentWasModified();
    void checkScintillaUndo();
    void treeActivated(const QModelIndex &index);
    void treeDestroyed();
    void logDestroyed();
    void logShowed();
    void viewQsci();
    void viewText();
    void viewHead();
    void viewTree();

    void clipboardDataChanged();
    void loadFinished(bool ok);
    void selectionChanged();
    void linesChanged();
    void undoChanged();
    void redoChanged();
    void showInspector();
    void zoomOrig();

private:
    bool loadXML(const QString &filename);
    QIcon icon(const QString &name);

private:
    void init();
    void createHead();
    void createTree();
    void createQsci();
    void createActions();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool maybeSave();
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName = QString());
    Fb2MainWindow *findFb2MainWindow(const QString &fileName);

    Fb2WebView *textEdit;
    QWebInspector *inspector;
    QTreeView *headTree;
    QTextEdit *noteEdit;
    QTextEdit *messageEdit;
    QDockWidget *dockTree;
    QsciScintilla *codeEdit;
    QTreeView *treeView;
    QString curFile;
    bool isUntitled;

    QToolBar *toolEdit;

    QMenu
        *menuEdit,
        *menuText,
        *menuView;

    QAction
        *actionBack,
        *actionForward,
        *actionUndo,
        *actionRedo,
        *actionCut,
        *actionCopy,
        *actionPaste,
        *actionSelect,
        *actionInsert,
        *actionDelete,
        *actionTextBold,
        *actionTextItalic,
        *actionTextStrike,
        *actionTextSub,
        *actionTextSup,
        *actionInspect,
        *actionZoomIn,
        *actionZoomOut,
        *actionZoomOrig
    ;
};

#endif // FB2MAIN_H
