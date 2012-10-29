#include "fb2dock.hpp"
#include "fb2code.hpp"
#include "fb2head.hpp"
#include "fb2text.hpp"

#include <QLayout>
#include <QtDebug>

//---------------------------------------------------------------------------
//  FbMainDock
//---------------------------------------------------------------------------

FbMainDock::FbMainDock(QWidget *parent)
    : QStackedWidget(parent)
    , isSwitched(false)
{
    textFrame = new FbWebFrame(this);
    m_text = new FbTextEdit(textFrame, parent);
    textFrame->layout()->addWidget(m_text);

    m_head = new FbHeadEdit(this);
    m_code = new FbCodeEdit(this);

    addWidget(textFrame);
    addWidget(m_head);
    addWidget(m_code);

    m_head->setText(m_text);
}

FbMainDock::Mode FbMainDock::mode() const
{
    QWidget * current = currentWidget();
    if (current == textFrame) return Text;
    if (current == m_head) return Head;
    if (current == m_code) return Code;
    return Text;
}

void FbMainDock::setMode(Mode mode)
{
    switch (mode) {
        case Text: setCurrentWidget(textFrame); return;
        case Head: setCurrentWidget(m_head); return;
        case Code: setCurrentWidget(m_code); return;
    }
}

bool FbMainDock::load(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCritical() << QObject::tr("Cannot read file %1: %2.").arg(filename).arg(file.errorString());
        return false;
    }

    if (currentWidget() == m_code) {
        m_code->clear();
        return m_code->read(&file);
    }

    return false;
}

bool FbMainDock::save(QIODevice *device, const QString &codec)
{
    if (currentWidget() == m_code) {
        QTextStream out(device);
        out << m_code->toPlainText();
    } else {
        isSwitched = false;
        m_text->save(device, codec);
    }
    return true;
}

