//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-8
//

#ifndef START_STOP_DAEMON_CMD_H
#define START_STOP_DAEMON_CMD_H

#include <QProcess>

namespace td {

class StartStopDaemonCmd : public QProcess {
    Q_OBJECT
    static QString gmProgramName;

public:
    enum StatusCode {
        Done,
        NothingDone,
        Trouble,
        WithRetry,
    };

    static void setProgramName(const QString &programName);
    explicit StartStopDaemonCmd(const QString &pidFile,
                                const QString &execFile,
                                QObject *parent = nullptr);
    StartStopDaemonCmd *withExecWorkingDirectory(const QString &workingDirectory);
    bool startDaemon();
    bool stopDaemon();

private:
    StatusCode execProcess(const QStringList &args);

private:
    QString mPidFile;
    QString mExecFile;
    QString mExecWorkingDirectory;
    QString mCommand;
    int mPid;
};

}// namespace td

#endif//START_STOP_DAEMON_CMD_H
