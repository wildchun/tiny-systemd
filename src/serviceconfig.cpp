//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-8
//

#include "serviceconfig.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <qfile.h>

namespace td {

static QStringList parseArgInputLine(const QString &arg) {
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

int readPidFromFile(const QString &pidFileName) {
    QFile pidFile(pidFileName);
    if (!pidFile.open(QIODevice::ReadOnly)) {
        return -1;
    }
    const QString pidString = QString::fromUtf8(pidFile.readAll()).trimmed();
    bool ok;
    const int pid = pidString.toInt(&ok);
    if (!ok) {
        return -1;
    }
    pidFile.close();
    if (pid <= 0) {
        qWarning() << "pid file content is not valid:" << pidString;
        return -1;
    }
    return pid;
}

// #define USE_KILL_PID
#define USE_PROC_DIR

bool isProcessAlive(int pid) {
#if defined(USE_KILL_PID)
    return ::kill(static_cast<pid_t>(pid), 0) == 0;
#endif
#if defined(USE_PROC_DIR)
    return QDir("/proc/" + QString::number(pid)).exists();
#endif
}

QString ServiceConfig::gServiceDirectory = "/etc/tiny_daemon/services";
QString ServiceConfig::gPidDirectory = "/var/run/tiny_daemon";

void ServiceConfig::setServiceDirectory(const QString &dir) {
    gServiceDirectory = dir;
}

void ServiceConfig::setPidDirectory(const QString &dir) {
    gPidDirectory = dir;
}

QString ServiceConfig::serviceDirectory() {
    return gServiceDirectory;
}

QString ServiceConfig::pidDirectory() {
    return gPidDirectory;
}

QString ServiceConfig::localServerName() {
    return "tiny_daemon.socket";
}

ServiceConfig::ServiceConfig() :
    mServiceType(Simple),
    mAutoRestartType(AutoRestartDisable),
    mServiceRestartSec(0),
    mEnabled(false), mPriority(0) {
}

ServiceConfig::~ServiceConfig() = default;

QString ServiceConfig::serviceName() const { return mServiceName; }

QString ServiceConfig::serviceFile() const { return mServiceFile; }

QString ServiceConfig::description() const {
    return mUnitDescription;
}

QString ServiceConfig::executedFile() const { return mServiceExecStart; }

QString ServiceConfig::contextFile() const {
        return mContextFileName;
}

QString ServiceConfig::workingDirectory() const { return mServiceWorkingDirectory; }

QStringList ServiceConfig::arguments() const {
    return mServiceArgs;
}

ServiceConfig::AutoRestartType ServiceConfig::autoRestartType() const {
    return mAutoRestartType;
}

int ServiceConfig::restartSec() const { return mServiceRestartSec; }

bool ServiceConfig::enabled() const { return mEnabled; }

QString ServiceConfig::pidLockFileName() const {
    return mPidLockFileName;
}

bool ServiceConfig::loadFromSettings(const QString &fileName) {
    static const QHash<QString, ServiceConfig::AutoRestartType> kAutoRestartTypeMap = {
            {"always", ServiceConfig::AutoRestartAlways},
            {"on-failure", ServiceConfig::AutoRestartOnFailure},
            {"on-abort", ServiceConfig::AutoRestartOnAbort}};

    static const QHash<QString, ServiceConfig::Type> kServiceTypeMap = {
            {"simple", ServiceConfig::Simple},
            {"forking", ServiceConfig::Forking},
            {"notify", ServiceConfig::Notify},
            {"oneshot", ServiceConfig::OneShot}};

    QSettings settings(fileName, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");

    if (settings.status() != QSettings::NoError) {
        mError = QString("File %1 is not valid").arg(fileName);
        return false;
    }

    mUnitDescription = settings.value("Unit/Description").toString();

    const auto serviceType = settings.value("Service/Type").toString();
    if (serviceType.isEmpty()) {
        mError = QString("Service Type is not valid");
        return false;
    }
    mServiceType = kServiceTypeMap.value(serviceType, ServiceConfig::Simple);

    const auto serviceExecStart = settings.value("Service/ExecStart").toString();
    // 解析参数 <bin> <arg1> <arg2>
    const auto args = parseArgInputLine(serviceExecStart);
    if (args.isEmpty()) {
        mError = QString("Service ExecStart is not valid");
        return false;
    }
    mServiceExecStart = args.at(0);
    if (args.size() > 1) {
        mServiceArgs = args.mid(1);
    }
    mServiceWorkingDirectory = settings.value("Service/WorkingDirectory", "/").toString();

    const QFileInfo fileInfo(mServiceExecStart);
    if (!fileInfo.exists() || !fileInfo.isExecutable()) {
        mError = QString("Service Bin %1 is not exist or not executable").arg(mServiceExecStart);
        return false;
    }
    // 重启策略
    const auto serviceRestart = settings.value("Service/Restart").toString();
    mServiceRestartSec = settings.value("Service/RestartSec", "5").toInt();
    mAutoRestartType = kAutoRestartTypeMap.value(serviceRestart, AutoRestartDisable);
    if (mAutoRestartType != AutoRestartDisable && mServiceRestartSec <= 0) {
        mError = QString("RestartSec %1 is not valid").arg(mServiceRestartSec);
        return false;
    }
    // 自动启动使能
    mEnabled = settings.value("Service/Enabled", "true").toString() == "true";

    mPriority = settings.value("Service/Priority", "0").toInt();

    //
    mServiceFile = fileName;
    mServiceName = QFileInfo(fileName).fileName();
    mPidLockFileName = QDir(gPidDirectory).absoluteFilePath(mServiceName + ".pid");
    mContextFileName = QDir(gPidDirectory).absoluteFilePath(mServiceName + ".context");
    mError.clear();
    return true;
}

bool ServiceConfig::isSame(const ServiceConfig &other) const {
    if (mUnitDescription != other.mUnitDescription) {
        return false;
    }
    if (mServiceType != other.mServiceType) {
        return false;
    }
    if (mServiceExecStart != other.mServiceExecStart) {
        return false;
    }
    if (mServiceWorkingDirectory != other.mServiceWorkingDirectory) {
        return false;
    }
    if (mServiceRestartSec != other.mServiceRestartSec) {
        return false;
    }
    if (mAutoRestartType != other.mAutoRestartType) {
        return false;
    }
    if (mEnabled != other.mEnabled) {
        return false;
    }
    if (mServiceFile != other.mServiceFile) {
        return false;
    }
    if (mServiceName != other.mServiceName) {
        return false;
    }
    if (mPriority != other.mPriority) {
        return false;
    }
    return true;
}

int ServiceConfig::priority() const { return mPriority; }

ServiceConfig::Type ServiceConfig::type() const {
    return mServiceType;
}

bool ServiceConfigLoader::load(const QString &fileName) {
    const QFile file(fileName);
    if (QFile::exists(fileName) == false) {
        mError = QString("File %1 does not exist").arg(fileName);
        return false;
    }
    ServiceConfig &config = mServiceConfig;
    if (config.loadFromSettings(fileName)) {
        return true;
    }
    mError = config.mError;
    return false;
}

const ServiceConfig &ServiceConfigLoader::serviceConfig() {
    return mServiceConfig;
}

const QString &ServiceConfigLoader::error() const {
    return mError;
}
}// namespace td