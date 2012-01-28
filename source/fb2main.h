#ifndef FB2MAIN_H
#define FB2MAIN_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QTextEdit;
class QTextDocument;
QT_END_NAMESPACE

class QsciScintilla;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    MainWindow(const QString &filename, QTextDocument * document = NULL);
    static QTextDocument * LoadDocument(const QString &filename);

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
    void init();
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool maybeSave();
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName, QTextDocument * document = NULL);
    QString strippedName(const QString &fullFileName);
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
