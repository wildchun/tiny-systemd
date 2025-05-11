//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-8
//

#include "startstopdaemoncmd.h"

#include "serviceconfig.h"

#include <QDebug>
#include <QFile>

/*
start-stop-daemon 1.9.18 for Debian - small and fast C version written by
Marek Michalkiewicz <marekm@i17linuxb.ists.pwr.wroc.pl>, public domain.

Usage:
  start-stop-daemon -S|--start options ... -- arguments ...
  start-stop-daemon -K|--stop options ...
  start-stop-daemon -H|--help
  start-stop-daemon -V|--version

Options (at least one of --exec|--pidfile|--user is required):
  -x|--exec <executable>        program to start/check if it is running
  -p|--pidfile <pid-file>       pid file to check
  -c|--chuid <name|uid[:group|gid]>
                change to this user/group before starting process
  -u|--user <username>|<uid>    stop processes owned by this user
  -n|--name <process-name>      stop processes with this name
  -s|--signal <signal>          signal to send (default TERM)
  -a|--startas <pathname>       program to start (default is <executable>)
  -r |--chroot <directory>                       chroot to <directory> before starting
  -d|--chdir <directory>                         change to <directory> (default is /)
  -N|--nicelevel <incr>         add incr to the process's nice level
  -b|--background               force the process to detach
  -m|--make-pidfile             create the pidfile before starting
  -R|--retry <schedule>         check whether processes die, and retry
  -t|--test                     test mode, don't do anything
  -o|--oknodo                   exit status 0 (not 1) if nothing done
  -q|--quiet                    be more quiet
  -v|--verbose                  be more verbose
Retry <schedule> is <item>|/<item>/... where <item> is one of
 -<signal-num>|[-]<signal-name>  send that signal
 <timeout>                       wait that many seconds
 forever                         repeat remainder forever
or <schedule> may be just <timeout>, meaning <signal>/<timeout>/KILL/<timeout>

Exit status:  0 = done      1 = nothing done (=> 0 if --oknodo)
              3 = trouble   2 = with --retry, processes wouldn't die

*/

namespace td {

QString StartStopDaemonCmd::gmProgramName = "start-stop-daemon";

void StartStopDaemonCmd::setProgramName(const QString &programName) {
    gmProgramName = programName;
}

QString StartStopDaemonCmd::programName() {
    return gmProgramName;
}

StartStopDaemonCmd::StartStopDaemonCmd(const QString &pidFile, const QString &execFile, QObject *parent) :
    QProcess(parent),
    mPidFile(pidFile),
    mExecFile(execFile) {
    setProgram(gmProgramName);
}

StartStopDaemonCmd *StartStopDaemonCmd::withExecWorkingDirectory(const QString &workingDirectory) {
    mExecWorkingDirectory = workingDirectory;
    return this;
}

StartStopDaemonCmd *StartStopDaemonCmd::withArgs(const QStringList &args) {
    mArgs = args;
    return this;
}

StartStopDaemonCmd::StatusCode StartStopDaemonCmd::execProcess(const QStringList &args) {
    QString cmd;
    QTextStream out(&cmd);
    out << program();
    for (const auto &arg: args) {
        out << " " << arg;
    }
    mCommand = cmd;
    setArguments(args);
    setProcessChannelMode(ForwardedChannels);
    setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    start();
    waitForStarted();
    waitForFinished();
    auto stdOutput = readAllStandardOutput();
    auto stdError = readAllStandardError();
    close();
    return static_cast<StatusCode>(exitCode());
}

bool StartStopDaemonCmd::startDaemon() {
    QStringList args;
    //sudo start-stop-daemon -S  --make-pidfile --pidfile /var/run/tiny_daemon/sample_service.pid  -b -x executable
    args << "--start"              // Q启动
         << "--make-pidfile"       // 保存pid文件
         << "--pidfile" << mPidFile//  pid路径
         << "--background"         //   后台运行
         << "--exec" << mExecFile;
    if (!mExecWorkingDirectory.isEmpty()) {
        args << "--chdir" << mExecWorkingDirectory;//  工作目录
    }
    if (!mArgs.isEmpty()) {
        args << "--";
        for (const auto &arg: mArgs) {
            args << arg;
        }
    }
    const auto code = execProcess(args);
    return code == Done || code == NothingDone;
}

bool StartStopDaemonCmd::stopDaemon() {
    QStringList args;
    args << "--stop"                // 停止
         << "--pidfile" << mPidFile;//  pid路径
    const auto code = execProcess(args);
    return code == Done || code == NothingDone;
}

QString StartStopDaemonCmd::executeCommand() const {
    { return mCommand; }
}

}// namespace td