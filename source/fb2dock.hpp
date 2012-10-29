#ifndef FB2DOCK_H
#define FB2DOCK_H

#include <QAction>
#include <QIcon>
#include <QStackedWidget>
#include <QToolBar>
#include <QIODevice>

class FbTextEdit;
class FbHeadEdit;
class FbCodeEdit;

class FbMainDock : public QStackedWidget
{
    Q_OBJECT

public:
    enum Mode { Text = 0, Head, Code };
    explicit FbMainDock(QWidget *parent = 0);
    FbTextEdit * text() { return m_text; }
    FbHeadEdit * head() { return m_head; }
    FbCodeEdit * code() { return m_code; }
    bool load(const QString &filename);
    bool save(QIODevice *device, const QString &codec = QString());
    Mode mode() const;
    void setMode(Mode mode);
    void setTool(QToolBar *tool) { m_tool = tool; }

signals:
    
public slots:

private:
    QFrame *textFrame;
    FbTextEdit *m_text;
    FbHeadEdit *m_head;
    FbCodeEdit *m_code;
    QToolBar *m_tool;
    bool isSwitched;
};

#endif // FB2DOCK_H
