#ifndef FB2MAIN_H
#define FB2MAIN_H

#include <QMainWindow>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QFile;
class QModelIndex;
class QThread;
class QTextEdit;
class QTreeView;
class QTextDocument;
class QWebView;
QT_END_NAMESPACE

class QsciScintilla;
class Fb2MainDocument;
class Fb2ReadThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow();
    explicit MainWindow(const QString &filename);
    explicit MainWindow(const QString &filename, Fb2MainDocument * document);

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    void logMessage(const QString &message);
    void sendDocument();

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

    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();
    void clipboardDataChanged();

private:
    bool loadXML(const QString &filename);
    void connectTextDocument(QTextDocument * document);
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
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    MainWindow *findMainWindow(const QString &fileName);

    Fb2ReadThread *thread;
    QWebView *textEdit;
    QTextEdit *noteEdit;
    QTextEdit *messageEdit;
    QsciScintilla *qsciEdit;
    QTreeView * treeView;
    QString curFile;
    bool isUntitled;

    QAction
        *actionUndo,
        *actionRedo,
        *actionCut,
        *actionCopy,
        *actionPaste,
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
