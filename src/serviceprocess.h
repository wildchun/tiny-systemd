//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-8
//

#ifndef SERVICE_PROCESS_H
#define SERVICE_PROCESS_H

#include "serviceconfig.h"

#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QObject>
#include <QTimer>

namespace td {
class ServiceProcess : public QObject {
    Q_OBJECT
public:
    enum Status {
        Stopped,///< 停止 / 程序终止
        Running,
        UserStopped,///< 用户停止
    };

    struct ProcStatus {
        int pid;       ///< pid号
        bool procAlive;///< 进程是否存活
    };
    Q_ENUM(Status)

    explicit ServiceProcess(const ServiceConfig &config, QObject *parent = nullptr);
    ~ServiceProcess() override;

    bool start();
    bool stop();
    bool restart();
    const ServiceConfig &serviceConfig() const;
    ProcStatus procStatus() const;
    void reload(const ServiceConfig &config);
    void exit();

    void setup();

private:
    bool innerStart() const;
    bool innerStop(int pid) const;
    void syncProcStatus();

    void autoRestartProcess();
    void onPidProcessDead(int pid);
    static ProcStatus loadProcStatus(const QString &pidFileName);

private:
    // Status mStatus;
    ServiceConfig mConf;
    ProcStatus mProcState;
    QTimer mAutoResumeTimer;
    QDateTime mExecLastModifyTime;
};

}// namespace td

#endif//SERVICE_PROCESS_H
