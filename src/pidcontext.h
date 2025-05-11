// **************************************************
// User : qinxuchun
// Date : 25-5-11 下午5:56
// Description : ...
//
// **************************************************

#ifndef PIDCONTEXT_H
#define PIDCONTEXT_H
#include <QDateTime>
#include <qstring.h>

namespace td {

class PidContext {
public:
    PidContext() = default;
    explicit PidContext(QString contextFile);
    void sync() const;
    void load();
    void clear();
    void incrementRestartCount();
    void setLastStartTime(const QDateTime &lastStartTime);

    int restartCount() const {
        return mRestartCount;
    }
    QDateTime lastStartTime() const {
        return mLastStartTime;
    }

private:
    QDateTime mLastStartTime;
    int mRestartCount{};
    QString mContextFile;
};

}// namespace td

#endif//PIDCONTEXT_H
