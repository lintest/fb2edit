#ifndef FB2MAIN_H
#define FB2MAIN_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QFile;
class QModelIndex;
class QTextEdit;
class QTreeView;
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
    void treeActivated(const QModelIndex &index);
    void treeDestroyed();
    void logDestroyed();
    void logShowed();
    void viewQsci();
    void viewText();

    void clipboardDataChanged();
    void selectionChanged();
    void undoChanged();
    void redoChanged();

private:
    bool loadXML(const QString &filename);
    QIcon icon(const QString &name);

private:
    void init();
    void createText();
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
    QTextEdit *noteEdit;
    QTextEdit *messageEdit;
    QsciScintilla *qsciEdit;
    QTreeView * treeView;
    QString curFile;
    bool isUntitled;

    QAction
        *actionBack,
        *actionForward,
        *actionUndo,
        *actionRedo,
        *actionCut,
        *actionCopy,
        *actionPaste,
        *actionSelect,
        *actionTextBold,
        *actionTextItalic,
        *actionTextStrike,
        *actionTextSub,
        *actionTextSup,
        *actionZoomIn,
        *actionZoomOut,
        *actionZoomOrig
    ;
};

#endif // FB2MAIN_H
