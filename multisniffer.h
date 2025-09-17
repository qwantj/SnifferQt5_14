#ifndef MULTISNIFFER_H
#define MULTISNIFFER_H

#include <QObject>
#include <QSocketNotifier>
#include <QDateTime>
#include <QVariantMap>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

class MultiSniffer : public QObject
{
    Q_OBJECT

public:
    explicit MultiSniffer(QObject* parent = nullptr);
    ~MultiSniffer();

public slots:
    void startSniff();
    void stopSniff();
    void resumeSniff();

signals:
    void packetCaptured(const QVariantMap &info);

private slots:
    void onReadyRead(int socketFd);

private:
    QVector<SOCKET>           sockets;
    QVector<QSocketNotifier*> notifiers;
    bool                      running = false;

    QVector<in_addr> enumLocalIPv4();
    void createSnifferFor(const in_addr& localAddr);
    void parsePacket(const char* data, int len);
};

#endif // MULTISNIFFER_H
