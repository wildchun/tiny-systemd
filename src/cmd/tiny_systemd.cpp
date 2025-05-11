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
    td::TinyDaemonServer server;
    return QCoreApplication::exec();
}