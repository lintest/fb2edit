#include <QErrorMessage>
#include <QLocale>
#include <QTranslator>

#include "fb2app.hpp"
#include "fb2logs.hpp"
#include "fb2main.hpp"

#ifndef PACKAGE_NAME
    #define PACKAGE_NAME "fb2edit"
    #define PACKAGE_VENDOR "LinTest"
    #define PACKAGE_VERSION "0.XX.XX"
#endif  // PACKAGE_VERSION

QString FbApplication::lastCommit()
{
#ifdef COMMIT_INFO
    return QString(COMMIT_INFO).replace("//", "<br>");
#else
    return QString();
#endif  // PACKAGE_VERSION
}

void FbApplication::handleMessage(QtMsgType type, const char *msg)
{
    emit logMessage(type, QString::fromUtf8(msg));
}

static void fb2MessageHandler(QtMsgType type, const char *msg)
{
    ((FbApplication*)qApp)->handleMessage(type, msg);
}

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(fb2edit);

    FbApplication app(argc, argv);
    app.setApplicationName(QString(PACKAGE_NAME));
    app.setOrganizationName(QString(PACKAGE_VENDOR));
    app.setApplicationVersion(QString(PACKAGE_VERSION));

    QTranslator translator;
    translator.load(QLocale::system().name(), ":ts");
    app.installTranslator(&translator);

    int count = app.arguments().count();
    for (int i = 1; i < count; ++i) {
        QString arg = app.arguments().at(i);
        (new FbMainWindow(arg))->show();
    }
    if (count == 1) (new FbMainWindow)->show();

    qInstallMsgHandler(fb2MessageHandler);

    return app.exec();
}
