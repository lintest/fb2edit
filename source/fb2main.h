#ifndef FB2MAIN_H
#define FB2MAIN_H

#include <QMainWindow>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QFile;
class QTextEdit;
class QTextDocument;
QT_END_NAMESPACE

class QsciScintilla;
class Fb2MainDocument;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow();
    explicit MainWindow(const QString &filename);
    explicit MainWindow(const QString &filename, QTextDocument * document);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void fileNew();
    void fileOpen();
    bool fileSave();
    bool fileSaveAs();

    void about();
    void documentWasModified();
    void viewQsci();
    void viewText();

    void textBold();
    void textUnder();
    void textItalic();

    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();
    void clipboardDataChanged();

private:
    static Fb2MainDocument * loadFB2(const QString &filename);
    bool loadXML(const QString &filename);
    void connectTextDocument(QTextDocument * document);

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
    void setCurrentFile(const QString &fileName, QTextDocument * document = NULL);
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    MainWindow *findMainWindow(const QString &fileName);

    QTextEdit *textEdit;
    QsciScintilla *qsciEdit;
    QString curFile;
    bool isUntitled;

    QAction
        *actionUndo,
        *actionRedo,
        *actionCut,
        *actionCopy,
        *actionPaste,
        *actionTextBold,
        *actionTextUnder,
        *actionTextItalic,
        *actionTextStrike,
        *actionTextSub,
        *actionTextSup;
};

#endif // FB2MAIN_H
