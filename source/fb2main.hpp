#ifndef FB2MAIN_H
#define FB2MAIN_H

#include <QMainWindow>
#include <QDockWidget>
#include <QListView>
#include <QXmlParseException>

QT_BEGIN_NAMESPACE
class QAction;
class QFile;
class QMenu;
class QModelIndex;
class QTextEdit;
class QTreeView;
class QWebInspector;
QT_END_NAMESPACE

class FbLogDock;

class FbMainDock;

#include "fb2logs.hpp"

class FbMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum ViewMode { FB2, XML };
    explicit FbMainWindow(const QString &filename = QString(), ViewMode mode = FB2);

protected:
    void closeEvent(QCloseEvent *event);

signals:
    void showInspectorChecked(bool);

public slots:
    void warning(int row, int col, const QString &msg);
    void error(int row, int col, const QString &msg);
    void fatal(int row, int col, const QString &msg);
    void logMessage(QtMsgType type, const QString &message);
    void status(const QString &text);

private slots:
    void fileNew();
    void fileOpen();
    bool fileSave();
    bool fileSaveAs();

    void about();
    void textChanged(bool modified);
    void logDestroyed();

    void openSettings();

private:
    QString appTitle() const;

private:
    void createHead();
    void createTree();
    void createImgs();
    void createActions();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool maybeSave();
    bool saveFile(const QString &fileName, const QString &codec = QString());
    void setCurrentFile(const QString &fileName = QString());
    FbMainWindow *findFbMainWindow(const QString &fileName);

    FbMainDock *mainDock;
    QTextEdit *noteEdit;
    QToolBar *toolEdit;
    FbLogDock *logDock;
    QString curFile;
    bool isSwitched;
    bool isUntitled;
};

#endif // FB2MAIN_H
