//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-8
//

#ifndef TINYDAEMONSERVER_H
#define TINYDAEMONSERVER_H

#include "serviceconfig.h"
#include <QHash>
#include <QLocalServer>
#include <QObject>
#include <serviceprocessdaemon.h>

namespace td {

class TinyDaemonServer : public QObject {
    Q_OBJECT
public:
    explicit TinyDaemonServer(QObject *parent = nullptr);
    ~TinyDaemonServer() override;

private:
    void prepareService();
    void startServer();
    void startDbus();
    void onServerNewConnection();
    void onSocketReadReady();
    void executeClientCommand(QLocalSocket *socket, const QStringList &args);
    void reload();

private Q_SLOTS:
    void onSystemPowerDownOrReboot();
    void onFeedDog();

private:
    static QHash<QString, ServiceConfig> loadServiceConfigs();

private:
    QHash<QString, ServiceProcessDaemon *> mServiceProcesses;
    QLocalServer *mServer;

    QTimer mFeedDogTimer;
};

}// namespace td

#endif//TINYDAEMONSERVER_H
