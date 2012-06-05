#include <QApplication>
#include <QErrorMessage>

#include "fb2app.hpp"
#include "fb2main.hpp"

void Fb2Application::handleMessage(QtMsgType type, const char *msg)
{
    /*
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s\n", msg);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", msg);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg);
        abort();
    }
    */
    emit logMessage( QString::fromUtf8(msg));
}

static void fb2MessageHandler(QtMsgType type, const char *msg)
{
    ((Fb2Application*)qApp)->handleMessage(type, msg);
}

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(fb2edit);

    Fb2Application app(argc, argv);
    app.setApplicationName("fb2edit");
    app.setOrganizationName("LinTest");

    QTranslator translator;
    translator.load(QLocale::system().name(), ":ts");
    app.installTranslator(&translator);

    Fb2MainWindow * mainWin = new Fb2MainWindow;
    mainWin->show();

    qInstallMsgHandler(fb2MessageHandler);

    return app.exec();
}
