#include "fb2utils.h"

#include <QAction>
#include <QFile>
#include <QTextStream>
#include <QToolBar>
#include <QWebView>

namespace FB2 {

QIcon icon(const QString &name)
{
    QIcon icon;
    icon.addFile(QString(":/24x24/%1.png").arg(name), QSize(24,24));
    icon.addFile(QString(":/16x24/%1.png").arg(name), QSize(16,16));
    return QIcon::fromTheme(name, icon);
}

QString read(const QString &filename)
{
    // TODO: throw an exception instead of
    // returning an empty string
    QFile file( filename );
    if (!file.open( QFile::ReadOnly)) return QString();

    QTextStream in( &file );

    // Input should be UTF-8
    in.setCodec( "UTF-8" );

    // This will automatically switch reading from
    // UTF-8 to UTF-16 if a BOM is detected
    in.setAutoDetectUnicode( true );

    return in.readAll();
}

void addTools(QToolBar *tool, QWebView *view)
{
    QAction *act;

    act = view->pageAction(QWebPage::Undo);
    act->setIcon(FB2::icon("edit-undo"));
    act->setText(QObject::tr("&Undo"));
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Undo);
    tool->addAction(act);

    act = view->pageAction(QWebPage::Redo);
    act->setIcon(FB2::icon("edit-redo"));
    act->setText(QObject::tr("&Redo"));
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Redo);
    tool->addAction(act);

    tool->addSeparator();

    act = view->pageAction(QWebPage::Cut);
    act->setIcon(FB2::icon("edit-cut"));
    act->setText(QObject::tr("Cu&t"));
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Cut);
    act->setStatusTip(QObject::tr("Cut the current selection's contents to the clipboard"));
    tool->addAction(act);

    act = view->pageAction(QWebPage::Copy);
    act->setIcon(FB2::icon("edit-copy"));
    act->setText(QObject::tr("&Copy"));
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Copy);
    act->setStatusTip(QObject::tr("Copy the current selection's contents to the clipboard"));
    tool->addAction(act);

    act = view->pageAction(QWebPage::Paste);
    act->setIcon(FB2::icon("edit-paste"));
    act->setText(QObject::tr("&Paste"));
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Paste);
    act->setStatusTip(QObject::tr("Paste the clipboard's contents into the current selection"));
    tool->addAction(act);

    tool->addSeparator();

    act = view->pageAction(QWebPage::ToggleBold);
    act->setIcon(FB2::icon("format-text-bold"));
    act->setText(QObject::tr("&Bold"));
    tool->addAction(act);

    act = view->pageAction(QWebPage::ToggleItalic);
    act->setIcon(FB2::icon("format-text-italic"));
    act->setText(QObject::tr("&Italic"));
    tool->addAction(act);

    act = view->pageAction(QWebPage::ToggleStrikethrough);
    act->setIcon(FB2::icon("format-text-strikethrough"));
    act->setText(QObject::tr("&Strikethrough"));
    tool->addAction(act);

    act = view->pageAction(QWebPage::ToggleSuperscript);
    act->setIcon(FB2::icon("format-text-superscript"));
    act->setText(QObject::tr("Su&perscript"));
    tool->addAction(act);

    act = view->pageAction(QWebPage::ToggleSubscript);
    act->setIcon(FB2::icon("format-text-subscript"));
    act->setText(QObject::tr("Su&bscript"));
    tool->addAction(act);
}

}
