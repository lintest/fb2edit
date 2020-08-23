#include "fb2dock.hpp"
#include "fb2code.hpp"
#include "fb2head.hpp"
#include "fb2page.hpp"
#include "fb2text.hpp"

#include <QLayout>
#include <QtDebug>

//---------------------------------------------------------------------------
//  FbModeAction
//---------------------------------------------------------------------------

FbModeAction::FbModeAction(FbMainDock *parent, Fb::Mode mode, const QIcon &icon, const QString &text)
    : QAction(icon, text, parent)
    , m_dock(parent)
    , m_mode(mode)
{
    setCheckable(true);
    connect(this, SIGNAL(triggered()), SLOT(switchMode()));
}

FbModeAction::FbModeAction(FbMainDock *parent, Fb::Mode mode, const QString &text)
    : QAction(text, parent)
    , m_dock(parent)
    , m_mode(mode)
{
    setCheckable(true);
    connect(this, SIGNAL(triggered()), SLOT(switchMode()));
}

void FbModeAction::switchMode()
{
    m_dock->switchMode(m_mode);
}

//---------------------------------------------------------------------------
//  FbMainDock
//---------------------------------------------------------------------------

FbMainDock::FbMainDock(QWidget *parent)
    : QStackedWidget(parent)
    , isSwitched(false)
{
    textFrame = new FbTextFrame(this);
    m_text = new FbTextEdit(textFrame, parent);
    textFrame->layout()->addWidget(m_text);

    m_head = new FbHeadEdit(this, m_text);

    m_code = new FbCodeEdit(this);

    addWidget(textFrame);
    addWidget(m_head);
    addWidget(m_code);

    connect(m_text->page(), SIGNAL(warning(int,int,QString)), parent, SLOT(warning(int,int,QString)));
    connect(m_text->page(), SIGNAL(error(int,int,QString)), parent, SLOT(error(int,int,QString)));
    connect(m_text->page(), SIGNAL(fatal(int,int,QString)), parent, SLOT(fatal(int,int,QString)));
    connect(m_text->page(), SIGNAL(error(int,int,QString)), SLOT(error(int,int)));
    connect(m_text->page(), SIGNAL(fatal(int,int,QString)), SLOT(error(int,int)));
    connect(m_text->page(), SIGNAL(status(QString)), parent, SLOT(status(QString)));
    connect(m_text, SIGNAL(modificationChanged(bool)), SLOT(textChanged(bool)));
    connect(m_head, SIGNAL(modificationChanged(bool)), SLOT(textChanged(bool)));
    connect(m_code, SIGNAL(modificationChanged(bool)), SLOT(textChanged(bool)));
    connect(m_head, SIGNAL(status(QString)), parent, SLOT(status(QString)));
    connect(m_code, SIGNAL(status(QString)), parent, SLOT(status(QString)));
    connect(this, SIGNAL(status(QString)), parent, SLOT(status(QString)));
}

void FbMainDock::switchMode(Fb::Mode mode)
{
    if (mode == m_mode) return;
    isSwitched = isModified();
    if (currentWidget() == m_code) {
        QString xml = m_code->toPlainText();
        switch (m_mode) {
            case Fb::Code: m_text->page()->read(xml); break;
            case Fb::Html: m_text->setHtml(xml, m_text->url()); break;
            default: ;
        }
    } else {
        switch (mode) {
            case Fb::Code: {
                QString xml; int anchor, focus;
                m_text->save(&xml, anchor, focus);
                m_code->setPlainText(xml);
                QTextCursor cursor = m_code->textCursor();
                if (anchor > 0) cursor.setPosition(anchor, QTextCursor::MoveAnchor);
                if (focus > 0) cursor.setPosition(focus, QTextCursor::KeepAnchor);
                m_code->setTextCursor(cursor);
            } break;
            case Fb::Html: {
                QString html = m_text->toHtml();
                m_code->setPlainText(html);
            } break;
            default: ;
        }
    }
    setMode(mode);
}

void FbMainDock::setMode(Fb::Mode mode)
{
    enableMenu(mode == Fb::Text);
    switch (m_mode = mode) {
        case Fb::Text: setModeText(); break;
        case Fb::Head: setModeHead(); break;
        case Fb::Code: setModeCode(); break;
        case Fb::Html: setModeHtml(); break;
    }
    m_actions[mode]->setChecked(true);
    emit status(QString());
}

void FbMainDock::setModeText()
{
    m_mode = Fb::Text;
    setCurrentWidget(textFrame);
    m_head->disconnectActions();
    m_code->disconnectActions();
    m_text->connectActions(m_tool);
    m_text->viewContents(true);
}

void FbMainDock::setModeHead()
{
    m_mode = Fb::Head;
    m_text->hideDocks();
    setCurrentWidget(m_head);
    m_text->disconnectActions();
    m_code->disconnectActions();
    m_head->connectActions(m_tool);
    m_head->updateTree();
}

void FbMainDock::setModeCode()
{
    m_mode = Fb::Code;
    m_text->hideDocks();
    setCurrentWidget(m_code);
    m_text->disconnectActions();
    m_head->disconnectActions();
    m_code->connectActions(m_tool);
}

void FbMainDock::setModeHtml()
{
    m_mode = Fb::Html;
    m_text->hideDocks();
    setCurrentWidget(m_code);
    m_text->disconnectActions();
    m_head->disconnectActions();
    m_code->connectActions(m_tool);
}

void FbMainDock::error(int row, int col)
{
    m_code->setCursor(row, col);
    setMode(Fb::Code);
}

bool FbMainDock::load(const QString &filename)
{
    QFile *file = new QFile(filename);
    if (!file->open(QFile::ReadOnly | QFile::Text)) {
        qCritical() << QObject::tr("Cannot read file %1: %2.").arg(filename).arg(file->errorString());
        delete file;
        return false;
    }

    if (currentWidget() == m_code) {
        m_code->clear();
        return m_code->read(file);
    } else {
        m_text->page()->read(file);
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

void FbMainDock::textChanged(bool changed)
{
    emit modificationChanged(isSwitched || changed);
}

bool FbMainDock::isModified() const
{
    if (isSwitched) return true;
    QWidget * current = currentWidget();
    if (current == textFrame) return m_text->isModified();
    if (current == m_head) return m_text->isModified();
    if (current == m_code) return m_code->isModified();
    return false;
}

void FbMainDock::addAction(Fb::Mode mode, QAction *action)
{
    m_actions.insert(mode, action);
}

void FbMainDock::addMenu(QMenu *menu)
{
    m_menus.append(menu);
}

void FbMainDock::enableMenu(bool value)
{
    for (QMenu *menu: m_menus) {
        menu->setEnabled(value);
    }
}
