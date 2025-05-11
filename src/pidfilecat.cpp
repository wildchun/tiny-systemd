// **************************************************
// User : qinxuchun
// Date : 25-5-11 下午2:43
// Description : ...
//
// **************************************************

#include "pidfilecat.h"

#include <QDir>
#include <QFile>

namespace td {
PidFileCat::PidFileCat(const QString &pidLockFileName) :
    mPidLockFileName(pidLockFileName),
    mState(NoPermissionAccess),
    mPid(0) {
}

void PidFileCat::setPidLockFileName(const QString &pidLockFileName) {
    mPidLockFileName = pidLockFileName;
}

void PidFileCat::sync() {
    QFile pidFile(mPidLockFileName);
    if (!pidFile.exists()) {
        mState = ProcessNotRunning;
        return;
    }
    if (!pidFile.open(QIODevice::ReadOnly)) {
        mState = NoPermissionAccess;
        return;
    }
    const QByteArray pidData = pidFile.readAll();
    pidFile.close();
    mPid = pidData.toInt();
    if (isPidProcAlive(mPid)) {
        mState = ProcessRunning;
    } else {
        mState = ProcessNotRunning;
        removePidFile();
    }
}

bool PidFileCat::isPidProcAlive(const int pid) {
    return QDir("/proc/" + QString::number(pid)).exists();
}

PidFileCat::State PidFileCat::state() const {
    return mState;
}

int PidFileCat::pid() const {
    return mPid;
}

void PidFileCat::removePidFile() const {
    if (QFile::exists(mPidLockFileName)) {
        QFile::remove(mPidLockFileName);
    }
}
}// namespace td