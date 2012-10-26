#include "fb2dock.hpp"
#include "fb2code.hpp"
#include "fb2head.hpp"
#include "fb2text.hpp"

FbMainDock::FbMainDock(QWidget *parent)
    : QStackedWidget(parent)
{
    addWidget(m_text = new FbTextEdit(this));
    addWidget(m_head = new FbHeadEdit(this));
    addWidget(m_code = new FbCodeEdit(this));
    m_head->setText(m_text);
}

FbMainDock::Mode FbMainDock::mode() const
{
    QWidget * current = currentWidget();
    if (current == m_text) return Text;
    if (current == m_head) return Head;
    if (current == m_code) return Code;
    return Text;
}

void FbMainDock::setMode(Mode mode)
{
}
