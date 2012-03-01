#ifndef FB2APP_H
#define FB2APP_H

#include <QtGui>

class Fb2Application : public QApplication
{
    Q_OBJECT

public:
    Fb2Application(int &argc, char **argv, int = QT_VERSION)
        : QApplication(argc, argv, QT_VERSION) {}

    void handleMessage(QtMsgType type, const char *msg);

signals:
    void logMessage(const QString &message);
};

#endif // FB2APP_H
