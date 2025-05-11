//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-8
//

#ifndef TD_SERVICE_CONFIG_H
#define TD_SERVICE_CONFIG_H

#include <QSettings>

namespace td {

int readPidFromFile(const QString &pidFileName);
bool isProcessAlive(int pid);

class ServiceConfigLoader;

class ServiceConfig {
    friend class ServiceConfigLoader;
    static QString gServiceDirectory;
    static QString gPidDirectory;

public:
    static void setServiceDirectory(const QString &dir);
    static void setPidDirectory(const QString &dir);


    static QString serviceDirectory();
    static QString pidDirectory();
    static QString localServerName();

    enum AutoRestartType {
        AutoRestartDisable,
        AutoRestartAlways,
        AutoRestartOnFailure,
        AutoRestartOnAbort
    };

    enum Type {
        Simple, // systemd 认为主进程启动后服务即运行，直接监控该进程。
        Forking,// 服务启动时会派生子进程，父进程退出后子进程继续运行。systemd 会监控子进程。
        Notify, // 服务启动时会向 systemd 发送通知，systemd 会监控该进程。
        OneShot,// 服务启动后退出，systemd 会监控该进程。
    };

    ServiceConfig();
    ~ServiceConfig();

    QString serviceName() const { return mServiceName; }

    QString executedFile() const { return mServiceExecStart; }

    QString workingDirectory() const { return mServiceWorkingDirectory; }

    AutoRestartType autoRestartType() const;

    int restartSec() const { return mServiceRestartSec; }

    bool enabled() const { return mEnabled; }

    QString pidLockFileName() const;

    bool loadFromSettings(const QString &fileName);

    bool isSame(const ServiceConfig &other) const;

    int priority() const;

    Type type() const;

private:
    QString mUnitDescription;
    Type mServiceType;
    QString mServiceExecStart;
    QString mServiceWorkingDirectory;
    AutoRestartType mAutoRestartType;
    int mServiceRestartSec;
    bool mEnabled;
    int mPriority;
    //
    QString mServiceFile;
    QString mServiceName;
    QString mPidLockFileName;
    QString mError;
};

class ServiceConfigLoader {
public:
    bool load(const QString &fileName);
    const ServiceConfig &serviceConfig();
    const QString &error() const;

private:
    QString mError;
    ServiceConfig mServiceConfig;
};

}// namespace td

#endif//TD_SERVICE_CONFIG_H
