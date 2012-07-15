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

class Fb2CodeEdit;
class Fb2TreeView;
class Fb2HeadView;
class Fb2WebFrame;
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
    void treeDestroyed();
    void logDestroyed();
    void logShowed();
    void viewCode();
    void viewText();
    void viewHead();
    void viewTree();

    void cleanChanged(bool clean);
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);
    void status(const QString &text);
    void clipboardDataChanged();
    void selectionChanged();
    void undoChanged();
    void redoChanged();
    void openSettings();

private:
    bool loadXML(const QString &filename);
    QString appTitle() const;

private:
    void init();
    void createHead();
    void createTree();
    void createActions();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool maybeSave();
    bool saveFile(const QString &fileName, const QString &codec = QString());
    void setCurrentFile(const QString &fileName = QString());
    Fb2MainWindow *findFb2MainWindow(const QString &fileName);

    Fb2WebFrame *textFrame;
    QWebInspector *inspector;
    Fb2HeadView *headTree;
    QTextEdit *noteEdit;
    QTextEdit *messageEdit;
    QDockWidget *dockTree;
    Fb2CodeEdit *codeEdit;
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
        *actionFind,
        *actionReplace,
        *actionImage,
        *actionNote,
        *actionLink,
        *actionBody,
        *actionTitle,
        *actionSubtitle,
        *actionDescr,
        *actionPoem,
        *actionStanza,
        *actionAuthor,
        *actionSection,
        *actionTextBold,
        *actionTextItalic,
        *actionTextStrike,
        *actionTextSub,
        *actionTextSup,
        *actionInspect,
        *actionZoomIn,
        *actionZoomOut,
        *actionZoomReset
    ;
};

#endif // FB2MAIN_H
