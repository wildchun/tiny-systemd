// **************************************************
// User : qinxuchun
// Date : 25-5-11 下午5:56
// Description : ...
//
// **************************************************

#include "pidcontext.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <utility>

namespace td {
PidContext::PidContext(QString contextFile) :
    mRestartCount(0),
    mContextFile(std::move(contextFile)) {
}

void PidContext::load() {
    clear();
    QFile contextFile(mContextFile);
    if (!contextFile.open(QIODevice::ReadOnly)) {
        return;
    }
    const QByteArray contextData = contextFile.readAll();
    contextFile.close();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(contextData);
    if (jsonDoc.isNull()) {
        return;
    }
    QJsonObject jsonObj = jsonDoc.object();
    mLastStartTime = QDateTime::fromString(jsonObj.value("lastStartTime").toString(), Qt::ISODateWithMs);
    mRestartCount = jsonObj.value("restartCount").toInt();
    if (mRestartCount < 0) {
        mRestartCount = 0;
    }
    if (mLastStartTime.isNull()) {
        mLastStartTime = QDateTime();
    }
}

void PidContext::clear() {
    mLastStartTime = QDateTime();
    mRestartCount = 0;
}

void PidContext::incrementRestartCount() {
    mRestartCount++;
}

void PidContext::setLastStartTime(const QDateTime &lastStartTime) {
    mLastStartTime = lastStartTime;
}

void PidContext::sync() const {
    QFile contextFile(mContextFile);
    if (!contextFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return;
    }
    QJsonObject jsonObj;
    jsonObj.insert("lastStartTime", mLastStartTime.toString(Qt::ISODateWithMs));
    jsonObj.insert("restartCount", mRestartCount);
    QJsonDocument jsonDoc(jsonObj);
    contextFile.write(jsonDoc.toJson(QJsonDocument::Compact));
    contextFile.close();
}

}// namespace td