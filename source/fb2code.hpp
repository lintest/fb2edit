#ifndef FB2CODE_H
#define FB2CODE_H

#include <QByteArray>
#include <QObject>
#include <QPlainTextEdit>
#include <QTextCharFormat>
#include <QColor>
#include <QTextEdit>
#include <QToolBar>

#include "fb2mode.h"

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
QT_END_NAMESPACE

class FbCodeEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    FbCodeEdit(QWidget *parent = 0);

    QAction * act(Fb::Actions index) const;
    void setAction(Fb::Actions index, QAction *action);
    void connectActions(QToolBar *tool);
    void disconnectActions();

    QString text() const { return toPlainText(); }

    bool read(QIODevice *device);

    void load(const QByteArray data)
        { setPlainText(QString::fromUtf8(data.data())); }

    bool findText(const QString &exp, QTextDocument::FindFlags options = {});

    bool isModified() const { return document()->isModified(); }

    void setCursor(int line, int column);

signals:
    void status(const QString &text);

protected:
    void resizeEvent(QResizeEvent *event);

private slots:
    void clipboardDataChanged();
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);
    void find();
    void validate();
    void zoomIn();
    void zoomOut();
    void zoomReset();

private:
    class LineNumberArea : public QWidget
    {
    public:
        LineNumberArea(FbCodeEdit *parent) : QWidget(parent) { editor = parent; }
        QSize sizeHint() const { return QSize(editor->lineNumberAreaWidth(), 0); }
    protected:
        void paintEvent(QPaintEvent *event) { editor->lineNumberAreaPaintEvent(event); }
    private:
        FbCodeEdit *editor;
    };

private:
    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void setZoomRatio(qreal ratio);

private:
    QWidget *lineNumberArea;
    FbActionMap m_actions;
    qreal zoomRatio;
    static qreal baseFontSize;
    static qreal zoomRatioMin;
    static qreal zoomRatioMax;
    friend class FbCodeEdit::LineNumberArea;
};

#endif // FB2CODE_H
