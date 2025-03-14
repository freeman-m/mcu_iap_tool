#include "serialporthandler.h"
#include <QDebug>

SerialPortHandler::SerialPortHandler(QObject *parent)
    : QObject(parent)
{
    serialPort = new QSerialPort(this);

    // 连接串口数据接收信号
    connect(serialPort, &QSerialPort::readyRead, this, &SerialPortHandler::handleReadyRead);

    // 连接串口错误信号（Qt 5.6.2 使用 error(QSerialPort::SerialPortError) 信号）
    connect(serialPort, static_cast<void(QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) {
            emit errorOccurred(serialPort->errorString());
        }
    });
}

SerialPortHandler::~SerialPortHandler()
{
    if (serialPort->isOpen()) {
        serialPort->close();
    }

    delete serialPort;
    delete dataProtocol;
}

QList<QString> SerialPortHandler::getAvailablePorts()
{
    QList<QString> ports;
    for (const QSerialPortInfo &port : QSerialPortInfo::availablePorts()) {
        ports.append(port.portName());
    }
    return ports;
}

bool SerialPortHandler::openPort(const QString &portName, qint32 baudRate)
{
    serialPort->setPortName(portName);
    serialPort->setBaudRate(baudRate);

    if (serialPort->open(QIODevice::ReadWrite)) {
        return true;
    } else {
        emit errorOccurred(serialPort->errorString());
        return false;
    }
}

void SerialPortHandler::closePort()
{
    if (serialPort->isOpen()) {
        serialPort->close();
    }
}

bool SerialPortHandler::isPortOpen() const
{
    return serialPort->isOpen();
}

void SerialPortHandler::setBaudRate(qint32 baudRate)
{
    serialPort->setBaudRate(baudRate);
}


bool SerialPortHandler::sendData(const QByteArray &data)
{
    if (!serialPort->isOpen()) {
        emit errorOccurred("串口未打开");
        return false;
    }

    // 使用 DataProtocol 打包数据
    QByteArray packet = dataProtocol->packData(data);

    // 发送数据
    if (serialPort->write(packet) == packet.size()) {
        return true;
    } else {
        emit errorOccurred("发送数据失败");
        return false;
    }
}

void SerialPortHandler::handleReadyRead()
{
    static QByteArray buffer;

    // 读取所有数据
    buffer.append(serialPort->readAll());

    while(buffer.size() >= 14)
    {
        QByteArray data = dataProtocol->unpackData(buffer);

        if (!data.isEmpty())
        {
            emit dataReceived(data); // 发送解包后的数据
            buffer.remove(0, data.size()+8);    // 将完整的一帧数据删除
        }
        else
        {
            buffer.remove(0, 1);
        }
    }
}
