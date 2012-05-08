#ifndef FB2VIEW_H
#define FB2VIEW_H

#include <QHash>
#include <QResizeEvent>
#include <QTimer>
#include <QThread>
#include <QWebView>

class Fb2BaseWebView : public QWebView
{
    Q_OBJECT

public:
    Fb2BaseWebView(QWidget* parent = 0)
        : QWebView(parent)
    {
          m_timer.setInterval(100);
          m_timer.setSingleShot(true);
          connect(&m_timer, SIGNAL(timeout()), SLOT(doResize()));
    }

protected slots:
    void doResize() {
        QResizeEvent event(size(), m_size);
        QWebView::resizeEvent(&event);
        QWebView::update();
    }

protected:
     void resizeEvent(QResizeEvent* event) {
          if (!m_timer.isActive()) m_size = event->oldSize();
          m_timer.start();
     }

private:
    QTimer m_timer;
    QSize m_size;
};

class Fb2WebView : public Fb2BaseWebView
{
    Q_OBJECT
public:
    explicit Fb2WebView(QWidget *parent = 0);
    virtual ~Fb2WebView();
    bool load(const QString &filename);

    bool UndoEnabled();
    bool RedoEnabled();
    bool CutEnabled();
    bool CopyEnabled();
    bool BoldChecked();
    bool ItalicChecked();
    bool StrikeChecked();
    bool SubChecked();
    bool SupChecked();

signals:
    
public slots:
    void file(QString name, QString path);
    void html(QString name, QString html);
/*
    void Undo();
    void Redo();
    void Cut();
    void Copy();
    void Paste();
    void Bold();
    void Italic();
    void Strike();
*/
    void zoomIn();
    void zoomOut();
    void zoomOrig();

private slots:
    void fixContents();

private:
    typedef QHash<QString, QString> StringHash;
    StringHash m_files;
    QThread *m_thread;
};

#endif // FB2VIEW_H
