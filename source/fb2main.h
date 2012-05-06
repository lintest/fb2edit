#ifndef FB2MAIN_H
#define FB2MAIN_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QFile;
class QModelIndex;
class QTextEdit;
class QThread;
class QTreeView;
QT_END_NAMESPACE

class QsciScintilla;
class Fb2MainDocument;
class Fb2ReadThread;
class Fb2WebView;

class Fb2MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit Fb2MainWindow();
    explicit Fb2MainWindow(const QString &filename);
    explicit Fb2MainWindow(const QString &filename, Fb2MainDocument * document);

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

    void textBold();
    void textItalic();
    void textStrike();
    void textSub();
    void textSup();

    void cursorPositionChanged();
    void clipboardDataChanged();
    void contentChanged();
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
    void setCurrentFile(const QString &fileName, const QString &html = QString());
    Fb2MainWindow *findFb2MainWindow(const QString &fileName);

    Fb2ReadThread *thread;
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
