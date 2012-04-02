#ifndef FB2TEXT_H
#define FB2TEXT_H

#include <QResizeEvent>
#include <QTextEdit>
#include <QTimer>

class Fb2TextEdit : public QTextEdit
{
    Q_OBJECT

public:
    Fb2TextEdit(QWidget* parent = 0)
        : QTextEdit(parent)
    {
          m_timer.setInterval( 100 );
          m_timer.setSingleShot(true);
          connect(&m_timer, SIGNAL(timeout()), SLOT(doResize()));
    }

protected slots:
    void doResize() {
        QResizeEvent event(size(), m_size);
        QTextEdit::resizeEvent(&event);
    }

protected:
     void resizeEvent( QResizeEvent* e ) {
          if (!m_timer.isActive()) m_size = e->oldSize();
          m_timer.start();
     }

private:
    QTimer m_timer;
    QSize m_size;
};

#endif // FB2TEXT_H
