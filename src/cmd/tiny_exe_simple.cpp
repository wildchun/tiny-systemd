// **************************************************
// User : qinxuchun
// Date : 25-5-11 下午5:11
// Description : ...
//
// **************************************************


#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QFile file("/home/qxc/code/github/tiny-systemd/z_output/start_record.txt");
    if (file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append)) {
        QTextStream out(&file);
        out << "start at " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << ", args" << QCoreApplication::arguments().join(" ") << " cwd = " << QDir::currentPath() << "\n";
        file.close();
    }
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, []() {
        qDebug() << "Hello, world!";
    });
    timer.start(1000);// 1 second interval
    return QCoreApplication::exec();
}