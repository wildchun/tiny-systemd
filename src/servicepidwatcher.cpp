//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-8
//

#include "servicepidwatcher.h"
#include <QDebug>
#include <QDir>

namespace td {

Q_GLOBAL_STATIC(ServicePidWatcher, gServicePidWatcher)

ServicePidWatcher *ServicePidWatcher::instance() {
    return gServicePidWatcher;
}

// static QString pidStatusFileName(const int pid) {
//     return QString("/proc/%1/status").arg(pid);
// }

static QString pidDirName(const int pid) {
    return QString("/proc/%1").arg(pid);
}

ServicePidWatcher::ServicePidWatcher() {
    mWatchTimer.setInterval(1000);
    connect(&mWatchTimer, &QTimer::timeout, this, &ServicePidWatcher::checkProcess, Qt::QueuedConnection);
    mWatchTimer.start();
}

void ServicePidWatcher::checkProcess() {
    if (mWatchedPaths.isEmpty()) {
        return;
    }
    QList<QString> deadPaths;
    for (auto it = mWatchedPaths.constBegin();
         it != mWatchedPaths.constEnd();
         ++it) {
        const auto dirName = it.key();
        if (QDir(dirName).exists()) {
            continue;
        }
        deadPaths.append(dirName);
        emit processDead(it.value());
    }
    for (const auto &path: deadPaths) {
        mWatchedPaths.remove(path);
    }
}

void ServicePidWatcher::addPid(const int pid) {
    if (pid <= 0) {
        return;
    }
    const auto pidDir = pidDirName(pid);
    if (mWatchedPaths.contains(pidDir)) {
        return;
    }
    mWatchedPaths.insert(pidDir, pid);
}

void ServicePidWatcher::removePid(int pid) {
    if (pid <= 0) {
        return;
    }
    const auto filename = pidDirName(pid);
    if (!mWatchedPaths.contains(filename)) {
        return;
    }
    mWatchedPaths.remove(filename);
}

}// namespace td