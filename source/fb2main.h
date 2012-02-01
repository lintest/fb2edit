#ifndef FB2MAIN_H
#define FB2MAIN_H

#include <QMainWindow>

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
    void newFile();
    void open();
    bool save();
    bool saveAs();
    void about();
    void documentWasModified();
    void viewQsci();
    void viewText();

private:
    static Fb2MainDocument * loadFB2(const QString &filename);
    bool loadXML(const QString &filename);

private:
    void init();
    void createText();
    void createQsci();
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool maybeSave();
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName, QTextDocument * document = NULL);
    MainWindow *findMainWindow(const QString &fileName);

    QTextEdit *textEdit;
    QsciScintilla *qsciEdit;
    QString curFile;
    bool isUntitled;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;
    QToolBar *fileToolBar;
    QToolBar *editToolBar;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *closeAct;
    QAction *exitAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *textAct;
    QAction *qsciAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
};

#endif // FB2MAIN_H
