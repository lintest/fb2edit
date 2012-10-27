#ifndef FB2MAIN_H
#define FB2MAIN_H

#include <QMainWindow>
#include <QDockWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QFile;
class QMenu;
class QModelIndex;
class QTextEdit;
class QTreeView;
class QWebInspector;
QT_END_NAMESPACE

class FbMainDock;
class FbCodeEdit;
class FbTreeView;
class FbHeadEdit;
class FbTextEdit;
class FbTextFrame;
class FbTextPage;

class FbLogDock: public QDockWidget
{
    Q_OBJECT

public:
    explicit FbLogDock(const QString &title, QWidget *parent = 0, Qt::WindowFlags flags = 0)
        : QDockWidget(title, parent, flags) {}

    QSize sizeHint() const {
        QSize sh = QDockWidget::sizeHint();
        sh.setHeight(40);
        return sh;
    }
};

class FbDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit FbDockWidget(const QString &title, QWidget *parent = 0, Qt::WindowFlags flags = 0);
};

class FbMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum ViewMode { FB2, XML };
    explicit FbMainWindow(const QString &filename = QString(), ViewMode mode = FB2);

public:
    FbTextPage * page();

protected:
    void closeEvent(QCloseEvent *event);

signals:
    void showInspectorChecked(bool);

public slots:
    void logMessage(const QString &message);

private slots:
    void fileNew();
    void fileOpen();
    bool fileSave();
    bool fileSaveAs();

    void about();
    void createTextToolbar();
    void documentWasModified();
    void checkScintillaUndo();
    void treeDestroyed();
    void imgsDestroyed();
    void logDestroyed();
    void logShowed();
    void viewCode();
    void viewHtml();
    void viewText(FbTextPage *page = 0);
    void viewHead();
    void viewTree();
    void viewImgs();

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
    QString appTitle() const;

private:
    void createHead();
    void createTree();
    void createImgs();
    void createActions();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    void setModified(bool modified);
    bool maybeSave();
    bool saveFile(const QString &fileName, const QString &codec = QString());
    void setCurrentFile(const QString &fileName = QString());
    FbMainWindow *findFbMainWindow(const QString &fileName);

    FbMainDock *mainDock;
    QTextEdit *noteEdit;
    QToolBar *toolEdit;
    QDockWidget *dockTree;
    QDockWidget *dockImgs;
    QWebInspector *inspector;
    QTextEdit *messageEdit;
    QString curFile;
    bool isSwitched;
    bool isUntitled;

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
        *actionPasteText,
        *actionSelect,
        *actionFind,
        *actionReplace,
        *actionImage,
        *actionNote,
        *actionLink,
        *actionBody,
        *actionTitle,
        *actionEpigraph,
        *actionSubtitle,
        *actionAnnot,
        *actionPoem,
        *actionDate,
        *actionStanza,
        *actionAuthor,
        *actionSection,
        *actionSimpleText,
        *actionParaSeparator,
        *actionLineSeparator,
        *actionClearFormat,
        *actionTextBold,
        *actionTextItalic,
        *actionTextStrike,
        *actionTextCode,
        *actionTextSub,
        *actionTextSup,
        *actionTextTitle,
        *actionSectionAdd,
        *actionSectionDel,
        *actionContents,
        *actionPictures,
        *actionInspect,
        *actionZoomIn,
        *actionZoomOut,
        *actionZoomReset
    ;
};

#endif // FB2MAIN_H
