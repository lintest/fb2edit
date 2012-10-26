#ifndef FB2APP_H
#define FB2APP_H

#include <QApplication>

class FbApplication : public QApplication
{
    Q_OBJECT

public:
    FbApplication(int &argc, char **argv, int = QT_VERSION)
        : QApplication(argc, argv, QT_VERSION) {}

    void handleMessage(QtMsgType type, const char *msg);

    static QString lastCommit();

signals:
    void logMessage(const QString &message);
};


#endif // FB2APP_H
