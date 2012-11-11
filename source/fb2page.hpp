#ifndef FB2PAGE_HPP
#define FB2PAGE_HPP

#include <QAction>
#include <QUndoCommand>
#include <QWebPage>

class FbStore;
class FbTextElement;
class FbNetworkAccessManager;

#include "fb2logs.hpp"
#include "fb2mode.h"

class FbTextLogger : public QObject
{
    Q_OBJECT

public:
    explicit FbTextLogger(QObject *parent = 0) : QObject(parent) {}

public slots:
    void trace(const QString &text);

};

class FbTextPage : public QWebPage
{
    Q_OBJECT

public:
    explicit FbTextPage(QObject *parent = 0);
    FbNetworkAccessManager *manager();
    bool read(const QString &html);
    bool read(QIODevice *device);
    void push(QUndoCommand * command, const QString &text = QString());
    FbTextElement element(const QString &location);
    FbTextElement current();
    QString location();

    FbTextElement body();
    FbTextElement doc();

    FbTextElement appendSection(const FbTextElement &parent);
    FbTextElement appendTitle(const FbTextElement &parent);
    FbTextElement appendText(const FbTextElement &parent);
    static QUrl createUrl();

signals:
    void status(const QString &text);
    void warning(int row, int col, const QString &msg);
    void error(int row, int col, const QString &msg);
    void fatal(int row, int col, const QString &msg);

public slots:
    void html(const QString &html, FbStore *store);
    void insertBody();
    void insertTitle();
    void insertAnnot();
    void insertAuthor();
    void insertEpigraph();
    void insertSubtitle();
    void insertSection();
    void insertPoem();
    void insertStanza();
    void insertDate();
    void insertText();
    void createSection();
    void deleteSection();
    void createTitle();

protected:
    virtual bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);
    void createBlock(const QString &name);

protected:
    static QString block(const QString &name);
    static QString block(const QString &name, const QString &text);
    static QString p(const QString &text = "<br/>");
    void update();

private slots:
    void loadFinished();
    void fixContents();
    void showStatus();

private:
    FbActionMap m_actions;
    FbTextLogger m_logger;
    QString m_html;
};

#endif // FB2PAGE_HPP
