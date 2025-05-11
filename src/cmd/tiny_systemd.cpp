//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-9
//
#include "tinydaemonserver.h"

#include <QCoreApplication>
#include <QProcessEnvironment>
#include <startstopdaemoncmd.h>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    // for test
    //
    td::ServiceConfig::setPidDirectory("/home/qxc/code/github/tiny-systemd/z_test_dir/pids");
    td::ServiceConfig::setServiceDirectory("/home/qxc/code/github/tiny-systemd/z_test_dir/services");
    td::StartStopDaemonCmd::setProgramName("/home/qxc/code/github/tiny-systemd/z_output/tinystartstopdaemon");
    td::TinyDaemonServer server;
    return app.exec();
}