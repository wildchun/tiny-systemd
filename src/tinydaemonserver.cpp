//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-8
//

#include "tinydaemonserver.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDebug>
#include <QDir>
#include <QLocalSocket>
#include <QProcess>
#include <startstopdaemoncmd.h>

namespace td {

QStringList parseArgInputLine(const QString &arg) {
    QStringList args;
    QString buf;
    bool inQuote = false;
    for (auto i = 0; i < arg.size(); ++i) {
        if (arg.at(i) == '\"') {
            inQuote = !inQuote;
            continue;
        }
        if (arg.at(i) == ' ' && !inQuote) {
            if (!buf.isEmpty()) {
                args.append(buf);
                buf.clear();
            }
            continue;
        }
        if (arg.at(i) == '\\') {
            i++;
            if (i >= arg.size()) {
                break;
            }
        }
        buf.append(arg.at(i));
    }
    if (inQuote) {
        return {};
    }
    if (!buf.isEmpty()) {
        args.append(buf);
    }
    return args;
}

TinyDaemonServer::TinyDaemonServer(QObject *parent) :
    QObject(parent),
    mServer(nullptr) {

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption("services_dir", "set services directory", "services_dir", "/etc/tiny_daemon/services"));
    parser.addOption(QCommandLineOption("pid_dir", "set pid directory", "pid_dir", "/var/run/tiny_daemon"));
    parser.addOption(QCommandLineOption("daemon", "set start-stop-daemon program", "daemon", "start-stop-daemon"));
    parser.parse(QCoreApplication::arguments());

    const auto servicesDir = parser.value("services_dir");
    const auto pidDir = parser.value("pid_dir");
    const auto daemon = parser.value("daemon");
    qDebug() << " -- use services_dir   :" << servicesDir << "\n"
             << " -- pid_dir            :" << pidDir << "\n"
             << " -- daemon program     :" << daemon;
    ServiceConfig::setServiceDirectory(servicesDir);
    ServiceConfig::setPidDirectory(pidDir);
    StartStopDaemonCmd::setProgramName(daemon);
    QTimer::singleShot(0, this, &TinyDaemonServer::prepareService);
    mFeedDogTimer.setInterval(30000);
    connect(&mFeedDogTimer, &QTimer::timeout, this, &TinyDaemonServer::onFeedDog, Qt::QueuedConnection);
    mFeedDogTimer.start();
}

TinyDaemonServer::~TinyDaemonServer() {
}

void TinyDaemonServer::prepareService() {
    const QDir servicesDir(ServiceConfig::serviceDirectory());
    const QDir pidDir(ServiceConfig::pidDirectory());

    if (!servicesDir.exists()) {
        qWarning() << servicesDir.path() << " does not exist, please create it";
        qApp->exit(1);
        return;
    }
    if (!pidDir.exists()) {
        qDebug() << pidDir.path() << " does not exist, now create it";
        if (!pidDir.mkpath(pidDir.path())) {
            qWarning() << pidDir.path() << "create failed, please try run server as root";
            qApp->exit(1);
            return;
        }
    }
    QFile daemonFile(pidDir.path() + "/tiny-daemon-server.pid");
    if (daemonFile.exists()) {
        const int pid = readPidFromFile(daemonFile.fileName());
        if (pid > 0 && isProcessAlive(pid)) {
            qWarning() << "tiny-daemon-server is already running, pid:" << pid;
            qApp->exit(1);
            return;
        }
    }
    if (!daemonFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        qWarning() << "can't read write file on " << pidDir.path() << " please try run server as root";
        qApp->exit(1);
        return;
    }
    daemonFile.write(QString::number(QCoreApplication::applicationPid()).toUtf8());
    daemonFile.close();
    if (!daemonFile.remove()) {
        qWarning() << "Failed to remove PID file:" << pidDir.path() + "/tiny-daemon-server.pid";
        qApp->exit(1);
        return;
    }
    startServer();
    reload();
    startDbus();
}

QHash<QString, ServiceConfig> TinyDaemonServer::loadServiceConfigs() {
    QHash<QString, ServiceConfig> serviceConfigs;
    const QDir configDir(ServiceConfig::serviceDirectory());
    if (!configDir.exists()) {
        return serviceConfigs;
    }
    const QStringList configFiles = configDir.entryList(QDir::Files);
    for (const QString &fileName: configFiles) {
        qDebug() << "Loading service config:" << fileName;
        if (!fileName.endsWith(".service")) {
            continue;
        }
        ServiceConfigLoader loader;
        if (!loader.load(configDir.absoluteFilePath(fileName))) {
            qDebug() << "Failed to load service config:" << fileName << "err:" << loader.error();
            continue;
        }
        const ServiceConfig &config = loader.serviceConfig();
        serviceConfigs.insert(config.serviceName(), config);
        qDebug() << "Loading service config:" << fileName << "success, server name:" << config.serviceName();
    }
    return serviceConfigs;
}

void TinyDaemonServer::reload() {
    QVector<ServiceProcess *> newProcesses;

    const auto newServiceConfigs = loadServiceConfigs();
    for (auto newIt = newServiceConfigs.constBegin();
         newIt != newServiceConfigs.constEnd();
         ++newIt) {
        const QString &serviceName = newIt.key();
        if (mServiceProcesses.contains(serviceName)) {
            const auto proc = mServiceProcesses.value(serviceName);
            proc->reload(newIt.value());
        } else {
            //新增
            qDebug() << "New service config:" << serviceName;
            const auto proc = new ServiceProcess(newIt.value());
            mServiceProcesses.insert(serviceName, proc);
            newProcesses.append(proc);
        }
    }
    for (auto it = mServiceProcesses.constBegin();
         it != mServiceProcesses.constEnd();
         ++it) {
        const QString &serviceName = it.key();
        if (!newServiceConfigs.contains(serviceName)) {
            qDebug() << "Remove service :" << serviceName;
            const auto proc = it.value();
            proc->exit();
            proc->deleteLater();
            mServiceProcesses.erase(it);
        }
    }
    if (newProcesses.isEmpty()) {
        return;
    }
    std::sort(newProcesses.begin(), newProcesses.end(), [](ServiceProcess *first, ServiceProcess *last) {
        return first->serviceConfig().priority() < last->serviceConfig().priority();
    });
    for (const auto proc: newProcesses) {
        proc->setup();
    }
}

void TinyDaemonServer::startServer() {
    QLocalServer::removeServer(ServiceConfig::localServerName());
    mServer = new QLocalServer(this);
    mServer->setSocketOptions(QLocalServer::WorldAccessOption);
    if (!mServer->listen(ServiceConfig::localServerName())) {
        qWarning() << "Failed to start server:" << mServer->errorString();
        qApp->exit(1);
        return;
    }
    connect(mServer, &QLocalServer::newConnection, this, &TinyDaemonServer::onServerNewConnection, Qt::QueuedConnection);
    qDebug() << "Server started, listening on:" << mServer->fullServerName();
}

void TinyDaemonServer::startDbus() {
    auto conn = QDBusConnection::connectToBus("unix:path=/tmp/default-session-bus", "tiny_daemon");
    if (!conn.isConnected()) {
        qWarning() << "Failed to connect to D-Bus session bus" << conn.lastError().message();
        return;
    }
    //dbus-send --session --type=signal /event com.hdapp.system.event.EventPowerDown
    // #模拟系统重启DBUS信号
    // dbus-send --session --type=signal /event com.hdapp.system.event.EventRebootLater
    const auto ret = conn.connect("", "/event",
                                  "com.hdapp.system.event",
                                  "EventPowerDown",
                                  this,
                                  SLOT(onSystemPowerDownOrReboot()));
    if (!ret) {
        qWarning() << "Failed to connect to D-Bus signal:" << conn.lastError().message();
        return;
    }
    const auto ret2 = conn.connect("", "/event",
                                   "com.hdapp.system.event",
                                   "EventRebootLater",
                                   this,
                                   SLOT(onSystemPowerDownOrReboot()));
    if (!ret2) {
        qWarning() << "Failed to connect to D-Bus signal:" << conn.lastError().message();
        return;
    }
    qInfo() << "Connected to EventPowerDown and EventRebootLater signals";
}

void TinyDaemonServer::onSystemPowerDownOrReboot() {
    qDebug() << "System power down or reboot signal received, exit now";
    qApp->exit(0);
}

void TinyDaemonServer::onFeedDog() {
    ::system("echo V > /dev/watchdog");
}

void TinyDaemonServer::onServerNewConnection() {
    while (mServer->hasPendingConnections()) {
        const QLocalSocket *socket = mServer->nextPendingConnection();
        if (socket) {
            connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater, Qt::QueuedConnection);
            connect(socket, &QLocalSocket::readyRead, this, &TinyDaemonServer::onSocketReadReady, Qt::QueuedConnection);
        }
    }
}

void TinyDaemonServer::onSocketReadReady() {
    const auto socket = qobject_cast<QLocalSocket *>(sender());
    if (!socket) {
        return;
    }
    const QByteArray data = socket->readAll();
    if (data.isEmpty()) {
        qDebug() << "No data received";
        return;
    }
    const auto args = parseArgInputLine(data);
    executeClientCommand(socket, args);
}

struct CmdHelper {
    QLocalSocket *socket;

    ~CmdHelper() {
        socket->flush();
        socket->close();
    }
};

void TinyDaemonServer::executeClientCommand(QLocalSocket *socket, const QStringList &args) {
    static const QString helpText = ""
                                    "Usage: tinysystemctl [command] service\n"
                                    "Commands:\n"
                                    "  start       Start a service\n"
                                    "  stop        Stop a service\n"
                                    "  restart     Restart a service\n"
                                    "  status      Show the status of a service\n";
    static const QStringList validCommands = {"start", "stop", "restart", "status", "list", "reload"};
    CmdHelper cmdHelper{socket};
    QTextStream out(socket);
    if (args.size() < 2) {
        out << helpText;
        return;
    }
    const QString command = args.at(1);
    if (!validCommands.contains(command)) {
        out << "Invalid command: " << command << "\n"
            << helpText;
        return;
    }
    if (command == "list") {
        auto idx = 0;
        for (auto it = mServiceProcesses.constBegin();
             it != mServiceProcesses.constEnd();
             ++it) {
            const auto proc = it.value();
            auto conf = proc->serviceConfig();
            out << "[" << ++idx << "] " << it.key() << "\n";
            out << "  name:" << conf.serviceName() << "\n";
            out << "  exec:" << conf.executedFile() << "\n";
            out << "  work dir:" << conf.workingDirectory() << "\n";
            out << "  enabled:" << conf.enabled() << "\n";
            out << "  restart interval:" << conf.restartSec() << "\n";
            out << "  pid:" << proc->procStatus().pid << "\n";
            out << "  alive:" << proc->procStatus().procAlive << "\n";
        }
        return;
    }
    if (command == "reload") {
        reload();
        out << "Service reloaded successfully\n";
        return;
    }
    if (args.size() < 3) {
        out << "Service name is required\n"
            << helpText;
        return;
    }
    auto serviceName = args.at(2);
    if (!serviceName.endsWith(".service")) {
        serviceName += ".service";
    }
    if (!mServiceProcesses.contains(serviceName)) {
        out << "Service not found: " << serviceName << "\n";
        return;
    }
    const auto proc = mServiceProcesses.value(serviceName);
    if (command == "start") {
        if (proc->start()) {
            out << "Service started successfully\n";
        } else {
            out << "Failed to start service\n";
        }
    } else if (command == "stop") {
        if (proc->stop()) {
            out << "Service stopped successfully\n";
        } else {
            out << "Failed to stop service\n";
        }
    } else if (command == "restart") {
        if (proc->restart()) {
            out << "Service restarted successfully\n";
        } else {
            out << "Failed to restart service\n";
        }
    } else if (command == "status") {
        const auto conf = proc->serviceConfig();
        out << "  name:" << conf.serviceName() << "\n";
        out << "  exec:" << conf.executedFile() << "\n";
        out << "  work dir:" << conf.workingDirectory() << "\n";
        out << "  enabled:" << conf.enabled() << "\n";
        out << "  restart interval:" << conf.restartSec() << "\n";
        out << "  pid:" << proc->procStatus().pid << "\n";
        out << "  alive:" << proc->procStatus().procAlive << "\n";
    } else {
        out << "Unknown command: " << command << "\n"
            << helpText;
    }
}


}// namespace td