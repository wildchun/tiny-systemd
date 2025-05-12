// **************************************************
// User : qinxuchun
// Date : 25-5-11 下午3:23
// Description : ...
//
// **************************************************

#include "serviceprocessdaemon.h"

#include <QDebug>
#include <QFileInfo>
#include <servicepidwatcher.h>
#include <startstopdaemoncmd.h>

#define svrLogD() qDebug().noquote() << "[" + mConf.serviceName() + "]"
#define svrLogI() qInfo().noquote() << "[" + mConf.serviceName() + "]"
#define svrLogW() qWarning().noquote() << "[" + mConf.serviceName() + "]"
#define svrLogE() qCritical().noquote() << "[" + mConf.serviceName() + "]"

namespace td {
ServiceProcessDaemon::ServiceProcessDaemon(const ServiceConfig &config, QObject *parent) :
    QObject(parent), mStatus(Inactive), mConf(config) {
    mPid = PidFileCat(mConf.pidLockFileName());
    mPidContext = PidContext(mConf.contextFile());
    mPidContext.load();

    mRestartTimer.setInterval(mConf.restartSec() * 1000);
    mRestartTimer.setSingleShot(true);
    connect(ServicePidWatcher::instance(), &ServicePidWatcher::processDead, this, &ServiceProcessDaemon::onPidDead, Qt::QueuedConnection);
    mExecFileLastModifiedTime = QFileInfo(mConf.executedFile()).lastModified();
    initMachine();
}

void ServiceProcessDaemon::setup() {
    if (!mMachine.isRunning()) {
        mMachine.start();
    }
}

void ServiceProcessDaemon::reload(const ServiceConfig &config) {
    if (mConf.isSame(config)) {
        auto newExecFileLastModifiedTime = QFileInfo(config.executedFile()).lastModified();
        if (mExecFileLastModifiedTime == newExecFileLastModifiedTime) {
            return;
        } else {
            svrLogI() << "reload by exec file modified";
        }
    } else {
        svrLogI() << "reload by config modified";
    }
    switch (mStatus) {
        case InDaemon: {
            ServicePidWatcher::instance()->removePid(mPid.pid());
            killProcess();
            mPid.removePidFile();
            newDaemonEvent(DaemonEventIdExitDaemonToStart);
        } break;
        case InStop: {
            if (!mConf.enabled() && config.enabled()) {
                // 修改了配置文件，enabled从false变为true
                newDaemonEvent(DaemonEventIdUserStart);
            }
        } break;
        default:
            break;
    }
    mConf = config;
}

void ServiceProcessDaemon::exit() {
    QString ret;
    QTextStream out(&ret);
    userStop(out);
    if (mMachine.isRunning()) {
        mMachine.stop();
    }
}

const ServiceConfig &ServiceProcessDaemon::serviceConfig() const {
    return mConf;
}

void ServiceProcessDaemon::userStop(QTextStream &out) {
    switch (mStatus) {
        case InSetup:
        case Inactive: {
            out << "service is in setup state, please retry later\n";
        }; break;
        case InStart: {
            out << "service is in start state, please retry later\n";
        }; break;
        case InRestart: {
            svrLogI() << "user stop";
            mRestartTimer.stop();
            newDaemonEvent(DaemonEventIdExitRestartToStop);
            out << "service stop successfully\n";
        }; break;
        case InDaemon: {
            svrLogI() << "user stop";
            ServicePidWatcher::instance()->removePid(mPid.pid());
            killProcess();
            mPid.removePidFile();
            newDaemonEvent(DaemonEventIdExitDaemonToStop);
            out << "service stop successfully\n";
        } break;
        case InStop: {
            out << "service is already in stop state\n";
        } break;
        case InTerminal: {
            out << "service is in terminal state, cannot control\n";
        }
    }
}

void ServiceProcessDaemon::userStart(QTextStream &out) {
    switch (mStatus) {
        case InSetup:
        case Inactive: {
            out << "service is in setup state, please retry later\n";
        }; break;
        case InStart: {
            out << "service is in start state, please retry later\n";
        }; break;
        case InRestart: {
            out << "service is in restart state, please stop first\n";
        }; break;
        case InDaemon: {
            out << "service is already in daemon state\n";
        } break;
        case InStop: {
            svrLogI() << "user start";
            out << "service start successfully\n";
            newDaemonEvent(DaemonEventIdUserStart);
        } break;
        case InTerminal: {
            out << "service is in terminal state, cannot control\n";
        }
    }
}

void ServiceProcessDaemon::userRestart(QTextStream &out) {
    switch (mStatus) {
        case InSetup:
        case Inactive: {
            out << "service is in setup state, please retry later\n";
        }; break;
        case InStart: {
            out << "service is in start state, please retry later\n";
        }; break;
        case InRestart: {
            out << "service is already in restart state\n";
        }; break;
        case InDaemon: {
            svrLogI() << "user restart";
            ServicePidWatcher::instance()->removePid(mPid.pid());
            killProcess();
            mPid.removePidFile();
            out << "service restart successfully\n";
            newDaemonEvent(DaemonEventIdExitDaemonToStart);
        } break;
        case InStop: {
            svrLogI() << "user restart";
            out << "service restart successfully\n";
            newDaemonEvent(DaemonEventIdUserStart);
        } break;
        case InTerminal: {
            out << "service is in terminal state, cannot control\n";
        }
    }
}

void ServiceProcessDaemon::status(QTextStream &out) {
    mPid.sync();
    out << ">> " << mConf.serviceName() << " - " << mConf.description() << "\n";
    out << "  Loaded: loaded (" << mConf.serviceFile() << "; " << (mConf.enabled() ? "enabled" : "disabled") << "; vendor preset: enabled)\n";
    out << "  Active: ";
    if (mPid.state() == PidFileCat::ProcessRunning) {
        out << "active (running) since " << mPidContext.lastStartTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
        out << "  Main PID: " << mPid.pid() << " (" << mConf.executedFile() << " " << mConf.arguments().join(' ') << ")\n";
    } else {
        switch (mStatus) {
            case Inactive:
                out << "inactive (dead, in terminal)\n";
                break;
            case InSetup:
                out << "inactive (dead, in setup)\n";
                break;
            case InStart:
                out << "inactive (dead, in restart)\n";
                break;
            case InRestart:
                out << "inactive (dead, wait restart)\n";
                break;
            case InDaemon:
                out << "inactive (dead, bug!!: in daemon)\n";
                break;
            case InStop:
                out << "inactive (dead, in stop mode)\n";
                break;
            case InTerminal:
                out << "inactive (dead, in terminal)\n";
                break;
        }
    }
}

void ServiceProcessDaemon::initMachine() {
    mMachine.addState(&mState.setup);
    mMachine.addState(&mState.restart);
    mMachine.addState(&mState.start);
    mMachine.addState(&mState.daemon);
    mMachine.addState(&mState.stop);
    mMachine.addState(&mState.terminal);
    mMachine.setInitialState(&mState.setup);

    connect(&mState.setup, &QState::entered, this, &ServiceProcessDaemon::onStaSetupEntry, Qt::QueuedConnection);
    connect(&mState.setup, &QState::exited, this, &ServiceProcessDaemon::onStaSetupExit, Qt::QueuedConnection);
    connect(&mState.restart, &QState::entered, this, &ServiceProcessDaemon::onStaRestartEntry, Qt::QueuedConnection);
    connect(&mState.restart, &QState::exited, this, &ServiceProcessDaemon::onStaRestartExit, Qt::QueuedConnection);
    connect(&mState.start, &QState::entered, this, &ServiceProcessDaemon::onStaStartEntry, Qt::QueuedConnection);
    connect(&mState.start, &QState::exited, this, &ServiceProcessDaemon::onStaStartExit, Qt::QueuedConnection);
    connect(&mState.daemon, &QState::entered, this, &ServiceProcessDaemon::onStaDaemonEntry, Qt::QueuedConnection);
    connect(&mState.daemon, &QState::exited, this, &ServiceProcessDaemon::onStaDaemonExit, Qt::QueuedConnection);
    connect(&mState.stop, &QState::entered, this, &ServiceProcessDaemon::onStaStopEntry, Qt::QueuedConnection);
    connect(&mState.stop, &QState::exited, this, &ServiceProcessDaemon::onStaStopExit, Qt::QueuedConnection);
    connect(&mState.terminal, &QState::entered, this, &ServiceProcessDaemon::onStaTerminalEntry, Qt::QueuedConnection);
    connect(&mState.terminal, &QState::exited, this, &ServiceProcessDaemon::onStaTerminalExit, Qt::QueuedConnection);

    mState.setup.addEventTransition(DaemonEventId::DaemonEventIdControllerDisable, &mState.terminal);
    mState.setup.addEventTransition(DaemonEventId::DaemonEventIdProcessRunning, &mState.daemon);
    mState.setup.addEventTransition(DaemonEventId::DaemonEventIdProcessDead, &mState.start);
    mState.setup.addEventTransition(DaemonEventId::DaemonEventIdProcessDeadButDisable, &mState.stop);

    mState.start.addEventTransition(DaemonEventId::DaemonEventIdStarted, &mState.daemon);
    mState.start.addEventTransition(DaemonEventId::DaemonEventIdStartFailed, &mState.restart);

    mState.restart.addTransition(&mRestartTimer, &QTimer::timeout, &mState.start);
    mState.restart.addEventTransition(DaemonEventId::DaemonEventIdAutoStartDisable, &mState.stop);
    mState.restart.addEventTransition(DaemonEventId::DaemonEventIdExitRestartToStop, &mState.stop);

    mState.daemon.addEventTransition(DaemonEventId::DaemonEventIdProcessDead, &mState.restart);
    mState.daemon.addEventTransition(DaemonEventId::DaemonEventIdProcessOneshotDead, &mState.terminal);
    mState.daemon.addEventTransition(DaemonEventId::DaemonEventIdExitDaemonToStop, &mState.stop);
    mState.daemon.addEventTransition(DaemonEventId::DaemonEventIdExitDaemonToStart, &mState.start);

    mState.stop.addEventTransition(DaemonEventId::DaemonEventIdUserStart, &mState.start);
}

void ServiceProcessDaemon::newDaemonEvent(const DaemonEventId id) {
    mMachine.postDaemonEvent(id);
}

void ServiceProcessDaemon::killProcess() const {
    StartStopDaemonCmd cmd(mConf.pidLockFileName(), mConf.executedFile());
    cmd.withExecWorkingDirectory(mConf.workingDirectory());
    cmd.withArgs(mConf.arguments());
    cmd.stopDaemon();
    // 冗余操作
    QProcess::execute("kill", QStringList() << "-9" << QString::number(mPid.pid()));
}

void ServiceProcessDaemon::onStaSetupEntry() {
    mStatus = InSetup;
    mPid.sync();
    switch (mPid.state()) {
        case PidFileCat::NoPermissionAccess: {
            svrLogE() << "is not controller able";
            newDaemonEvent(DaemonEventIdControllerDisable);
        } break;
        case PidFileCat::ProcessNotRunning: {
            svrLogI() << "is not running";
            if (mConf.enabled()) {
                newDaemonEvent(DaemonEventIdProcessDead);
            } else {
                newDaemonEvent(DaemonEventIdProcessDeadButDisable);
            }
        } break;
        case PidFileCat::ProcessRunning: {
            svrLogI() << "is running";
            newDaemonEvent(DaemonEventIdProcessRunning);
        } break;
    }
}

void ServiceProcessDaemon::onStaSetupExit() {}

void ServiceProcessDaemon::onStaStartEntry() {
    mStatus = InStart;
    StartStopDaemonCmd cmd(mConf.pidLockFileName(), mConf.executedFile());
    cmd.withExecWorkingDirectory(mConf.workingDirectory());
    cmd.withArgs(mConf.arguments());
    cmd.startDaemon();
    svrLogD() << "exec:" << cmd.executeCommand() << "code:" << cmd.exitCode();
    //过100 ms后，检查进程是否启动成功
    QTimer::singleShot(100, [=]() {
        mPid.sync();
        if (mPid.state() == PidFileCat::ProcessRunning) {
            svrLogD() << "start success";
            mPidContext.incrementRestartCount();
            mPidContext.setLastStartTime(QDateTime::currentDateTime());
            mPidContext.sync();
            newDaemonEvent(DaemonEventIdStarted);
        } else {
            newDaemonEvent(DaemonEventIdStartFailed);
        }
    });
}

void ServiceProcessDaemon::onStaStartExit() {}

void ServiceProcessDaemon::onStaRestartEntry() {
    mStatus = InRestart;
    if (mConf.autoRestartType() != ServiceConfig::AutoRestartType::AutoRestartDisable) {
        svrLogI() << "try to restart after" << mConf.restartSec() << "seconds";
        //如果是自动重启
        mRestartTimer.setInterval(mConf.restartSec() * 1000);
        mRestartTimer.start();
    } else {
        svrLogI() << "dead, but restart is disable, to stop state";
        newDaemonEvent(DaemonEventIdAutoStartDisable);
    }
}

void ServiceProcessDaemon::onStaRestartExit() {
    mRestartTimer.stop();
}

void ServiceProcessDaemon::onStaDaemonEntry() {
    mStatus = InDaemon;
    ServicePidWatcher::instance()->addPid(mPid.pid());
}

void ServiceProcessDaemon::onPidDead(const int pid) {
    mPid.sync();
    if (mPid.state() == PidFileCat::ProcessRunning) {
        return;
    }
    svrLogW() << "daemon" << pid << "is dead";
    if (mConf.type() == ServiceConfig::Type::OneShot) {
        svrLogI() << "one shot finished";
        newDaemonEvent(DaemonEventIdProcessOneshotDead);
        return;
    }
    newDaemonEvent(DaemonEventIdProcessDead);
}

void ServiceProcessDaemon::onStaDaemonExit() {
}

void ServiceProcessDaemon::onStaStopEntry() {
    mStatus = InStop;
    svrLogI() << "stop mode";
}

void ServiceProcessDaemon::onStaStopExit() {
}

void ServiceProcessDaemon::onStaTerminalEntry() {
    mStatus = InTerminal;
    svrLogI() << "terminal mode";
}

void ServiceProcessDaemon::onStaTerminalExit() {
}


}// namespace td