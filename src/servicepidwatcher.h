//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-8
//

#ifndef SERVICE_PID_WATCHER_H
#define SERVICE_PID_WATCHER_H

#include <QFileSystemWatcher>
#include <QObject>
#include <QSet>
#include <QTimer>

namespace td {

class ServicePidWatcher : public QObject {
    Q_OBJECT
public:
    ServicePidWatcher();
    static ServicePidWatcher *instance();
    void addPid(int pid);
    void removePid(int pid);
Q_SIGNALS:
    void processDead(int pid);
private:
    void checkProcess();
    QHash<QString, int> mWatchedPaths;
    QTimer mWatchTimer;
};

}// namespace td

#endif//SERVICE_PID_WATCHER_H
