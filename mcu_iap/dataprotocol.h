#ifndef DATAPROTOCOL_H
#define DATAPROTOCOL_H

#include <QObject>
#include <QByteArray>

class DataProtocol : public QObject
{
    Q_OBJECT

public:
    explicit DataProtocol(QObject *parent = nullptr);

    // 打包数据
    QByteArray packData(const QByteArray &data);

    // 解包数据
    QByteArray unpackData(const QByteArray &packet);

private:
    // 计算crc16
    uint16_t crc16(const uint8_t *p_buff, uint32_t len);

    // 解析数据
    void iap_decode_proc(void);
};

#endif // DATAPROTOCOL_H
