#include "MultiSniffer.h"
#include <QDebug>

#pragma pack(push, 1)
struct IPHeader {
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    quint8 ihl:4;
    quint8 version:4;
#else
    quint8 version:4;
    quint8 ihl:4;
#endif
    quint8  tos;
    quint16 tot_len;
    quint16 id;
    quint16 frag_off;
    quint8  ttl;
    quint8  protocol;
    quint16 check;
    quint32 saddr;
    quint32 daddr;
};
#pragma pack(pop)

MultiSniffer::MultiSniffer(QObject* parent)
    : QObject(parent)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        qFatal("WSAStartup failed");
    }
}

MultiSniffer::~MultiSniffer()
{
    for (auto nb : notifiers) {
        delete nb;
    }
    for (auto s : sockets) {
        closesocket(s);
    }
    WSACleanup();
}

void MultiSniffer::startSniff()
{
    if (running) return;
    auto addrs = enumLocalIPv4();
    if (addrs.isEmpty()){
        qWarning() <<"Нет локальных адресов";
        return;
    }

    for (auto& addr : addrs)
        createSnifferFor(addr);

    running = true;
    qDebug() << "Sniffer started";
}

void MultiSniffer::stopSniff()
{
    if (!running) return;

    for (auto* n : notifiers)
        n->setEnabled(false);

    DWORD off = RCVALL_OFF, out=0;
    for (auto s : sockets)
        WSAIoctl(s, SIO_RCVALL, &off, sizeof(off), nullptr, 0, &out, nullptr, nullptr);

    running = false;
    qDebug() << "Sniffing stopped";
}

void MultiSniffer::resumeSniff()
{
    if (running) return;


    for (auto* n : notifiers)
        n->setEnabled(true);

    DWORD on = RCVALL_ON, out=0;
    for (auto s : sockets)
        WSAIoctl(s, SIO_RCVALL, &on, sizeof(on), nullptr, 0, &out, nullptr, nullptr);

    running = true;
    qDebug() << "Sniffing resumed";
}

QVector<in_addr> MultiSniffer::enumLocalIPv4()
{
    QVector<in_addr> result;
    ULONG flags   = GAA_FLAG_INCLUDE_PREFIX;
    ULONG bufSize = 0;
    GetAdaptersAddresses(AF_INET, flags, nullptr, nullptr, &bufSize);
    if (bufSize == 0) return result;

    std::unique_ptr<BYTE[]> buffer(new BYTE[bufSize]);
    auto* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.get());
    if (GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &bufSize) != NO_ERROR) {
        return result;
    }

    for (auto* ad = adapters; ad; ad = ad->Next) {
        for (auto* ua = ad->FirstUnicastAddress; ua; ua = ua->Next) {
            auto* sa = reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
            result.append(sa->sin_addr);
        }
    }
    return result;
}

void MultiSniffer::createSnifferFor(const in_addr& localAddr)
{
    SOCKET s = ::socket(AF_INET, SOCK_RAW, IPPROTO_IP);
    if (s == INVALID_SOCKET) {
        qWarning() << "socket() failed:" << WSAGetLastError();
        return;
    }

    sockaddr_in bindAddr{};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr   = localAddr;
    bindAddr.sin_port   = 0;

    if (::bind(s, reinterpret_cast<sockaddr*>(&bindAddr),
               sizeof(bindAddr)) == SOCKET_ERROR) {
        qWarning() << "bind() failed:" << WSAGetLastError();
        closesocket(s);
        return;
    }

    DWORD enabled = RCVALL_ON, out = 0;
    if (WSAIoctl(s, SIO_RCVALL, &enabled, sizeof(enabled),
                 nullptr, 0, &out, nullptr, nullptr) == SOCKET_ERROR) {
        qWarning() << "WSAIoctl(SIO_RCVALL) failed:" << WSAGetLastError();
        closesocket(s);
        return;
    }

    u_long nonBlk = 1;
    ioctlsocket(s, FIONBIO, &nonBlk);

    auto* notifier = new QSocketNotifier(int(s),
                                         QSocketNotifier::Read,
                                         this);
    connect(notifier, &QSocketNotifier::activated,
            this, [this](int fd){ onReadyRead(fd); });

    sockets.append(s);
    notifiers.append(notifier);

    qDebug().noquote()
        << "Sniffer started on"
        << inet_ntoa(localAddr);
}

void MultiSniffer::onReadyRead(int socketFd)
{
    char buffer[65536];
    int len = recv(socketFd, buffer, sizeof(buffer), 0);
    if (len > 0) {
        parsePacket(buffer, len);
    }
}

void MultiSniffer::parsePacket(const char* data, int len)
{
    if (len < int(sizeof(IPHeader)))
        return;

    // Разбор IPv4-заголовка
    auto* ip = reinterpret_cast<const IPHeader*>(data);
    int ipLen = ip->ihl * 4;
    if (len < ipLen)
        return;

    quint32 s = ntohl(ip->saddr), d = ntohl(ip->daddr);
    QString src = QString("%1.%2.%3.%4")
                      .arg((s>>24)&0xFF).arg((s>>16)&0xFF)
                      .arg((s>>8)&0xFF).arg(s&0xFF);
    QString dst = QString("%1.%2.%3.%4")
                      .arg((d>>24)&0xFF).arg((d>>16)&0xFF)
                      .arg((d>>8)&0xFF).arg(d&0xFF);

    QVariantMap info;
    info["timestamp"] = QDateTime::currentDateTime();
    info["src"]       = src;
    info["dst"]       = dst;

    // TCP
    if (ip->protocol == IPPROTO_TCP) {
        if (len < ipLen + 20)
            return;

        const quint8* tcp = reinterpret_cast<const quint8*>(data + ipLen);
        quint16 srcPort = ntohs(*reinterpret_cast<const quint16*>(tcp + 0));
        quint16 dstPort = ntohs(*reinterpret_cast<const quint16*>(tcp + 2));
        quint8  dataOffset = (tcp[12] >> 4) * 4;
        int payloadOffset = ipLen + dataOffset;
        int payloadLen    = len - payloadOffset;
        if (payloadLen <= 0)
            return;
        const QByteArray payload(data + payloadOffset, payloadLen);

        info["srcPort"] = srcPort;
        info["dstPort"] = dstPort;
        info["length"]  = payloadLen;

        // HTTPS
        if (dstPort == 443 || srcPort == 443) {
            const quint8* p = reinterpret_cast<const quint8*>(payload.constData());
            if (payloadLen >= 5 &&
                p[0] == 0x16 &&
                p[1] == 0x03 && (p[2] <= 0x04))
            {
                info["protocol"] = "HTTPS";
                emit packetCaptured(info);
                return;
            }
        }

        // HTTP
        static const QList<QByteArray> httpSignatures = {
            "GET ", "POST ", "PUT ",   "DELETE ",
            "HEAD ", "OPTIONS ", "TRACE ", "CONNECT ",
            "PATCH ", "HTTP/1."
        };

        for (const auto& sig : httpSignatures) {
            if (payload.startsWith(sig)) {
                info["protocol"] = "HTTP";
                info["data"]     = QString::fromUtf8(payload.left(512));
                emit packetCaptured(info);
                return;
            }
        }

        // TCP
        info["protocol"] = "TCP";
        emit packetCaptured(info);

    }
    // UDP
    else if (ip->protocol == IPPROTO_UDP) {
        if (len < ipLen + 8)
            return;

        const quint16* ups =
            reinterpret_cast<const quint16*>(data + ipLen);
        quint16 srcPort = ntohs(ups[0]);
        quint16 dstPort = ntohs(ups[1]);
        int payloadLen  = len - (ipLen + 8);

        info["srcPort"]  = srcPort;
        info["dstPort"]  = dstPort;
        info["length"]   = payloadLen;
        info["protocol"] = "UDP";

        emit packetCaptured(info);
    }
}
