#include "PacketModel.h"
#include <QDateTime>

PacketModel::PacketModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int PacketModel::rowCount(const QModelIndex &) const
{
    return packets.size();
}

int PacketModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

QVariant PacketModel::data(const QModelIndex &index,
                           int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};

    const auto &p = packets.at(index.row());
    switch (index.column()) {
    case TimeCol:
        return p.value("timestamp")
            .toDateTime()
            .toString("HH:mm:ss.zzz");
    case ProtoCol:
        return p.value("protocol");
    case SrcCol:
        return p.value("src");
    case SrcPortCol:
        return p.value("srcPort");
    case DstCol:
        return p.value("dst");
    case DstPortCol:
        return p.value("dstPort");
    case LengthCol:
        return p.value("length");
    default:
        return {};
    }
}

QVariant PacketModel::headerData(int section,
                                 Qt::Orientation orientation,
                                 int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case TimeCol:    return "Время";
    case ProtoCol:   return "Протокол";
    case SrcCol:     return "Источник";
    case SrcPortCol: return "Порт от";
    case DstCol:     return "Назначение";
    case DstPortCol: return "Порт до";
    case LengthCol:  return "Длина";
    default:         return {};
    }
}

void PacketModel::appendPacket(const QVariantMap &info)
{
    const int newRow = packets.size();
    beginInsertRows(QModelIndex(), newRow, newRow);
    packets.append(info);
    endInsertRows();
}

QVariantMap PacketModel::packetAt(int row) const
{
    return packets.value(row);
}
