// **************************************************
// User : qinxuchun
// Date : 25-5-11 下午3:23
// Description : ...
//
// **************************************************

#ifndef SERVICEPROCESSDAEMON_H
#define SERVICEPROCESSDAEMON_H

#include <QAbstractTransition>
#include <QDateTime>
#include <QEvent>
#include <QFinalState>
#include <QObject>
#include <QStateMachine>
#include <QTimer>
#include <pidcontext.h>
#include <pidfilecat.h>
#include <qdatetime.h>
#include <qtextstream.h>
#include <serviceconfig.h>

namespace td {


enum DaemonEventId {
    DaemonEventIdControllerDisable = 0,
    DaemonEventIdProcessRunning,
    DaemonEventIdProcessDead,
    DaemonEventIdProcessDeadButDisable,


    DaemonEventIdStartFailed,
    DaemonEventIdStarted,

    DaemonEventIdAutoStartDisable,
    DaemonEventIdProcessOneshotDead,


    DaemonEventIdUserStart,
    DaemonEventIdUserStop,
    DaemonEventIdUserRestart,
};
class DaemonEventTransition;

class DaemonEvent : public QEvent {
    friend DaemonEventTransition;

public:
    explicit DaemonEvent(const DaemonEventId &id) :
        QEvent(static_cast<QEvent::Type>(QEvent::User + 1)),
        mEventId(id) {}

private:
    DaemonEventId mEventId;
};

class DaemonEventTransition : public QAbstractTransition {
private:
    Q_OBJECT
public:
    explicit DaemonEventTransition(const DaemonEventId id) :
        mEventId(id) {}

protected:
    bool eventTest(QEvent *e) override {
        if (e->type() != static_cast<QEvent::Type>(QEvent::User + 1))// StringEvent
            return false;
        const DaemonEvent *se = static_cast<DaemonEvent *>(e);
        return se->mEventId == mEventId;
    }

    void onTransition(QEvent *event) override{};

private:
    DaemonEventId mEventId;
};

class DaemonState : public QState {
    Q_OBJECT
public:
    using QState::QState;

    void addEventTransition(const DaemonEventId id, QState *target) {
        auto t = new DaemonEventTransition(id);
        t->setTargetState(target);
        QState::addTransition(t);
    }
};

class DaemonMachine : public QStateMachine {
    Q_OBJECT
public:
    using QStateMachine::QStateMachine;

    void postDaemonEvent(const DaemonEventId id) {
        postEvent(new DaemonEvent(id));
    }
};

class ServiceProcessDaemon : public QObject {
    Q_OBJECT
public:
    explicit ServiceProcessDaemon(const ServiceConfig &config, QObject *parent = nullptr);

    void setup();
    void reload(const ServiceConfig &config);
    void exit();
    const ServiceConfig &serviceConfig() const;
    void userStop(QTextStream &out);
    void userStart(QTextStream &out);
    void userRestart(QTextStream &out);
    void status(QTextStream &out);

private:
    void initMachine();
    void newDaemonEvent(const DaemonEventId id);
private Q_SLOTS:
    void onStaSetupEntry();
    void onStaSetupExit();
    void onStaRestartEntry();
    void onStaRestartExit();
    void onStaStartEntry();
    void onStaStartExit();
    void onStaDaemonEntry();
    void onStaDaemonExit();
    void onStaStopEntry();
    void onStaStopExit();
    void onStaTerminalEntry();
    void onStaTerminalExit();

private Q_SLOTS:
    void onPidDead(int pid);

private:
    ServiceConfig mConf;
    DaemonMachine mMachine;
    PidFileCat mPid;
    PidContext mPidContext;

    struct {
        DaemonState setup;
        DaemonState restart;
        DaemonState start;
        DaemonState daemon;
        DaemonState stop;
        DaemonState terminal;
    } mState;

    QTimer mRestartTimer;
};

}// namespace td

#endif//SERVICEPROCESSDAEMON_H
