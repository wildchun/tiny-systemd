//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-8
//

#include "serviceprocess.h"

#include "servicepidwatcher.h"
#include "startstopdaemoncmd.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <signal.h>

namespace td {


#define svrLogD() qDebug() << mConf.serviceName()
#define svrLogI() qInfo() << mConf.serviceName()
#define svrLogW() qWarning() << mConf.serviceName()
#define svrLogE() qCritical() << mConf.serviceName()

ServiceProcess::ProcStatus ServiceProcess::loadProcStatus(const QString &pidFileName) {
    auto sta = ProcStatus();
    QFile pidFile(pidFileName);
    if (!pidFile.open(QIODevice::ReadOnly)) {
        return sta;
    }
    const QString pidString = QString::fromUtf8(pidFile.readAll()).trimmed();
    bool ok;
    const int pid = pidString.toInt(&ok);
    if (!ok) {
        return sta;
    }
    pidFile.close();
    sta.pid = pid;
    sta.procAlive = isProcessAlive(sta.pid);
    return sta;
}

ServiceProcess::ServiceProcess(const ServiceConfig &config, QObject *parent) :
    QObject(parent),
    // mStatus(Stopped),
    mConf(config),
    mProcState() {
}

void ServiceProcess::setup() {
    svrLogI() << "Setup";
    const QDir pidDir(ServiceConfig::pidDirectory());

    mExecLastModifyTime = QFileInfo(mConf.executedFile()).lastModified();
    syncProcStatus();
    // 重启策略
    if (mConf.autoRestartType() == ServiceConfig::AutoRestartDisable) {
        svrLogI() << " auto restart enabled";
    } else {
        connect(&mAutoResumeTimer, &QTimer::timeout, this, &ServiceProcess::autoRestartProcess, Qt::QueuedConnection);
        mAutoResumeTimer.setInterval(mConf.restartSec() * 1000);
        svrLogI() << " auto restart enabled, interval:" << mConf.restartSec() << "seconds";
    }

    const auto watcher = ServicePidWatcher::instance();
    connect(watcher, &ServicePidWatcher::processDead, this, &ServiceProcess::onPidProcessDead);
    if (mProcState.procAlive) {
        watcher->addPid(mProcState.pid);
        svrLogI() << "is running, pid:" << mProcState.pid;
    } else {
        svrLogI() << "is not running";
        if (mConf.enabled()) {
            if (!start()) {
                svrLogI() << "start failed, try to restart after" << mConf.restartSec() << "seconds";
                if (mConf.autoRestartType() != ServiceConfig::AutoRestartDisable) {
                    mAutoResumeTimer.start();
                }
            } else {
                svrLogI() << "start success";
            }
        } else {
            svrLogI() << "is not enabled";
        }
    }
}

ServiceProcess::~ServiceProcess() {
}

ServiceProcess::ProcStatus ServiceProcess::procStatus() const { return mProcState; }

void ServiceProcess::reload(const ServiceConfig &config) {
    if (mConf.serviceName() != config.serviceName()) {
        svrLogW() << "service name not match";
        return;
    }
    const auto newExecInfo = QFileInfo(config.executedFile());
    if (newExecInfo.lastModified() == mExecLastModifyTime) {
        svrLogD() << "file not modified";
        if (mConf.isSame(config)) {
            svrLogD() << "config not modified";
            return;
        }
        svrLogD() << "config changed";
    } else {
        svrLogD() << "executeFile changed";
    }
    svrLogI() << "reload by config";
    mExecLastModifyTime = newExecInfo.lastModified();
    mConf = config;
    restart();
}

void ServiceProcess::exit() {
    stop();
    svrLogI() << "exit";
}

bool ServiceProcess::innerStart() const {
    StartStopDaemonCmd cmd(mConf.pidLockFileName(), mConf.executedFile());
    return cmd.withExecWorkingDirectory(mConf.workingDirectory())->startDaemon();
}

bool ServiceProcess::innerStop(const int pid) const {
    QProcess::execute("kill", QStringList() << "-9" << QString::number(pid));
    QFile::remove(mConf.pidLockFileName());
    return true;
}

bool ServiceProcess::start() {
    syncProcStatus();
    if (mProcState.procAlive) {
        return true;
    }
    if (!innerStart()) {
        svrLogI() << "start failed";
        return false;
    }
    syncProcStatus();
    const auto watcher = ServicePidWatcher::instance();
    watcher->addPid(mProcState.pid);
    mAutoResumeTimer.stop();
    svrLogI() << "start success, pid:" << mProcState.pid;
    return true;
}

bool ServiceProcess::stop() {
    syncProcStatus();
    if (!mProcState.procAlive) {
        return false;
    }
    // 在停止的时候，取消监听pid
    const auto watcher = ServicePidWatcher::instance();
    watcher->removePid(mProcState.pid);
    if (innerStop(mProcState.pid)) {
        svrLogI() << "stop success";
        syncProcStatus();
        mAutoResumeTimer.stop();
        return true;
    }
    svrLogI() << "stop failed";
    syncProcStatus();
    if (!mProcState.procAlive) {
        return false;
    }
    watcher->addPid(mProcState.pid);
    return false;
}

bool ServiceProcess::restart() {
    syncProcStatus();
    if (!mProcState.procAlive) {
        return start();
    }
    if (!stop()) {
        return false;
    }
    if (!start()) {
        return false;
    }
    return true;
}

const ServiceConfig &ServiceProcess::serviceConfig() const {
    return mConf;
}

void ServiceProcess::onPidProcessDead(const int pid) {
    if (pid != mProcState.pid) {
        return;
    }
    syncProcStatus();
    if (mConf.autoRestartType() != ServiceConfig::AutoRestartDisable) {
        svrLogD() << "dead, try to restart after" << mConf.restartSec() << "seconds";
        mAutoResumeTimer.start();
    } else {
        svrLogD() << "is dead";
    }
}

void ServiceProcess::syncProcStatus() {
    mProcState = loadProcStatus(mConf.pidLockFileName());
}

void ServiceProcess::autoRestartProcess() {
    syncProcStatus();
    if (mProcState.procAlive) {
        return;
    }
    start();
}
}// namespace td

// ps -ef | grep sample_service
//./tinysystemctl status hdapp.system.service
//./tinysystemctl stop hdapp.system.service
//./tinysystemctl start hdapp.system.service