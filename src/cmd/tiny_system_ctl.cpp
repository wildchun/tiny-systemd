//
// Author : XUCHUN QIN (0411)
// Date   : 25-5-9
//

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <serviceconfig.h>

class Client : public QObject {
    Q_OBJECT
public:
    Client() {
        QTimer::singleShot(0, this, &Client::run);
    }

    void doRequest() {
        const auto args = QCoreApplication::arguments();
        if (args.size() < 2) {
            qWarning() << "No arguments provided";
            return;
        }
        const QScopedPointer<QLocalSocket> socket(new QLocalSocket(this));
        socket->connectToServer(td::ServiceConfig::localServerName());
        if (!socket->waitForConnected(2000)) {
            qWarning() << "Failed to connect to server:" << socket->errorString();
            return;
        }
        const QString argsString = args.join(' ');
        socket->write(argsString.toUtf8());
        socket->waitForBytesWritten(2000);
        QTextStream out(stdout);
        while (socket->state() == QLocalSocket::ConnectedState) {
            if (!socket->waitForReadyRead(10000)) {
                continue;
            }
            const QByteArray response = socket->readAll();
            out << response;
            out.flush();
        }
        out << "\n";
        out.flush();
    }

    void run() {
        doRequest();
        qApp->exit(1);
    }
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    Client c;
    return app.exec();
}

#include "tiny_system_ctl.moc"
