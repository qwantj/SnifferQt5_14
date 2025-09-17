#ifndef PACKETMODEL_H
#define PACKETMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QVariantMap>

class PacketModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit PacketModel(QObject *parent = nullptr);

    enum Columns {
        TimeCol,
        ProtoCol,
        SrcCol,
        SrcPortCol,
        DstCol,
        DstPortCol,
        LengthCol,
        ColumnCount
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

public slots:
    void appendPacket(const QVariantMap &info);

public:
    QVariantMap packetAt(int row) const;

private:
    QVector<QVariantMap> packets;
};

#endif // PACKETMODEL_H
