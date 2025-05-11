// **************************************************
// User : qinxuchun
// Date : 25-5-11 下午2:43
// Description : ...
//
// **************************************************

#ifndef PIDFILECAT_H
#define PIDFILECAT_H
#include <QCommandLineOption>

namespace td {

class PidFileCat {
public:
    enum State {
        NoPermissionAccess,// 没有权限访问
        ProcessRunning,    // 进程不存在
        ProcessNotRunning, // 进程存在
    };

    PidFileCat() = default;
    explicit PidFileCat(const QString &pidLockFileName);
    void setPidLockFileName(const QString &pidLockFileName);
    void sync();
    static bool isPidProcAlive(int pid);
    State state() const;
    int pid() const;
    void removePidFile() const;

private:
    QString mPidLockFileName;
    State mState;
    int mPid;
};

}// namespace td

#endif//PIDFILECAT_H
