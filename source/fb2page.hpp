#ifndef FB2PAGE_HPP
#define FB2PAGE_HPP

#include <QAction>
#include <QUndoCommand>
#include <QWebPage>

class FbTextElement;
class FbNetworkAccessManager;

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
    FbNetworkAccessManager *temp();
    bool load(const QString &filename, const QString &xml = QString());
    void push(QUndoCommand * command, const QString &text = QString());
    FbTextElement element(const QString &location);
    FbTextElement current();
    QString location();
    QString status();

    FbTextElement body();
    FbTextElement doc();

    FbTextElement appendSection(const FbTextElement &parent);
    FbTextElement appendTitle(const FbTextElement &parent);

public slots:
    void html(const QString &html, const QUrl &url);
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
    void onTimer();
    void binary(const QString &name, const QByteArray &data);
    void loadFinished();
    void fixContents();

private:
    FbActionMap m_actions;
    FbTextLogger m_logger;
    QString m_html;
};

#endif // FB2PAGE_HPP
