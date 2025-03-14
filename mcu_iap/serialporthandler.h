#ifndef SERIALPORTHANDLER_H
#define SERIALPORTHANDLER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QList>
#include "dataprotocol.h"

class SerialPortHandler : public QObject
{
    Q_OBJECT

public:
    explicit SerialPortHandler(QObject *parent = nullptr);
    ~SerialPortHandler();

    // 获取可用串口列表
    QList<QString> getAvailablePorts();

    // 打开串口
    bool openPort(const QString &portName, qint32 baudRate);

    // 关闭串口
    void closePort();

    // 检查串口是否打开
    bool isPortOpen() const;

    // 设置波特率
    void setBaudRate(qint32 baudRate);

    // 发送数据
    bool sendData(const QByteArray &data);

    QSerialPort *serialPort;

signals:
    // 串口错误信号
    void errorOccurred(const QString &error);

    // 接收到数据
    void dataReceived(const QByteArray &data);

private slots:
    // 处理串口接收到的数据
    void handleReadyRead();

private:
//    QSerialPort *serialPort;

    DataProtocol *dataProtocol; // 协议处理对象
};

#endif // SERIALPORTHANDLER_H
