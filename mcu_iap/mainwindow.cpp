#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>

#include <QSettings>
#include <QFile>

#include <QFileDialog>

#include <cstring>

#include <QDateTime>

uint8_t index=0;

// 配置文件保存
void MainWindow::save_windows_parm()
{
    qDebug("%s", __func__);

    settings->setValue("serial/com", ui->comboBox_port->currentText());
    settings->setValue("serial/baudrate", ui->comboBox_baudrate->currentText());

    settings->setValue("file/path", ui->lineEdit_bin_path->text());

    settings->setValue("coeff/value", ui->lineEdit_pulse_voltage->text());

    settings->sync();
}

// 配置文件加载
void MainWindow::read_windows_parm()
{
    qDebug("%s", __func__);
    settings = new QSettings("setting.ini",QSettings::IniFormat);
    qDebug() << QCoreApplication::applicationDirPath();

    ui->comboBox_port->addItem(settings->value("serial/com").toString());
    ui->comboBox_baudrate->setCurrentText(settings->value("serial/baudrate").toString());

    QString savedPath = settings->value("file/path", "").toString();
    if (!savedPath.isEmpty())
    {
        ui->lineEdit_bin_path->setText(savedPath);
    }

    QString readString = settings->value("coeff/value", "").toString();
    if (!savedPath.isEmpty())
    {
        ui->lineEdit_pulse_voltage->setText(readString);
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    serialPortHandler(new SerialPortHandler(this)),
    timeoutTimer(new QTimer(this))
{
    ui->setupUi(this);

    // 初始化定时器
    timeoutTimer->setInterval(100); // 100ms 超时
    timeoutTimer->setSingleShot(true); // 单次触发
    connect(timeoutTimer, &QTimer::timeout, this, &MainWindow::onTimeout);

    read_windows_parm();

    connect(serialPortHandler, &SerialPortHandler::dataReceived, this, &MainWindow::onDataReceived);
    connect(serialPortHandler, &SerialPortHandler::dataReceivedStr, this, &MainWindow::onDataReceivedStr);
    // 连接串口错误信号
    connect(serialPortHandler, &SerialPortHandler::errorOccurred, this, &MainWindow::handleSerialPortError);

    ui->pushButton_start_updata->setEnabled(false);

    ui->lineEdit_coeff->setReadOnly(true);  // 设置为只读，用户不可编辑

    // 隐藏部分控件
    ui->frame->setHidden(false);

    ui->pushButton_set_unixtime->setHidden(true);
}

MainWindow::~MainWindow()
{
    save_windows_parm();

    delete ui;
}

// 刷新串口并排序
void MainWindow::on_pushButton_refresh_clicked()
{
    ui->comboBox_port->clear();
    QList<QString> ports = serialPortHandler->getAvailablePorts();

    qDebug() << "ports: " << ports;

    //冒泡排序法进行排序
    for (int i = 0; i < ports.size();i++)
    {
        for (int j = i + 1; j < ports.size(); j++)
        {
            QString name = ports.at(i);	//i的串口数字
            int portNumI = name.mid(3).toInt();

            name = ports.at(j);			//j的串口数字
            int portNumJ = name.mid(3).toInt();

            if (portNumI > portNumJ)				//ij交换
            {
                ports.swap(i, j);
            }
        }
    }

    for (const QString port : ports) {
        ui->comboBox_port->addItem(port);
    }
}


// 打开关闭串口
void MainWindow::on_pushButton_open_clicked()
{
    if (serialPortHandler->isPortOpen()) {
        serialPortHandler->closePort();
        ui->pushButton_open->setText("Open");
    }
    else
    {
        QString portName = ui->comboBox_port->currentText();
        qint32 baudRate = ui->comboBox_baudrate->currentText().toInt();
        if (serialPortHandler->openPort(portName, baudRate))
        {
            ui->pushButton_open->setText("Close");
        }
        else
        {
            QMessageBox::critical(this, "Error", "Failed to open serial port!");
        }
    }
}

// 串口错误提示
void MainWindow::handleSerialPortError(const QString &error)
{
    QMessageBox::critical(this, "Serial Port Error", error);
    if (serialPortHandler->isPortOpen())
    {
        serialPortHandler->closePort();
        ui->pushButton_open->setText("Open");
    }
}

// 选择bin文件
void MainWindow::on_pushButton_select_file_clicked()
{
    // 打开文件选择对话框
    QString filePath = QFileDialog::getOpenFileName(
        this,                               // 父窗口
        "选择 BIN 文件",                    // 对话框标题
        ui->lineEdit_bin_path->text().isEmpty() ? "QDir::homePath()" : ui->lineEdit_bin_path->text(),           //"",                                 // 默认打开路径（空字符串表示当前目录）
        "BIN 文件 (*.bin);;所有文件 (*)"     // 文件过滤器
    );

    // 如果用户选择了文件
    if (!filePath.isEmpty())
    {
        // 在 QLineEdit 中显示文件路径
        ui->lineEdit_bin_path->setText(filePath);

        // 可以在这里添加处理 BIN 文件的代码
        QMessageBox::information(this, "文件已选择", "选择的文件路径: " + filePath);

        file = new QFile(filePath);
        if (!file->open(QIODevice::ReadOnly))
        {
            QMessageBox::critical(this, tr("错误"), tr("无法打开文件"));
            return;
        }

        fileSize = file->size();
        fileData = file->readAll();
        bin_crc = static_cast<quint8>(fileData.at(fileData.size() - 2)) |
                (static_cast<quint8>(fileData.at(fileData.size() - 1)) << 8);
        qDebug("filesize: %d, bin_crc: 0x%04x.", fileSize, bin_crc);

        bytesWritten = 0;
        ui->progressBar->setMaximum(fileSize);
        ui->progressBar->setValue(0);
    }
    else
    {
        QMessageBox::warning(this, "未选择文件", "未选择任何文件！");
    }
}

// 上位机控制指令
#define	D_IAP_CMD_GET_DEVICE_INFO		0x10
#define	D_IAP_CMD_REBOOT				0x11

#define	D_IAP_CMD_IAP_REQ				0x20
#define	D_IAP_CMD_SEND_IAP_INFO			0x21
#define	D_IAP_CMD_SEND_IAP_DATA			0x22
#define	D_IAP_CMD_GET_IAP_DATA_CRC		0x23
#define	D_IAP_CMD_SEND_CRC_RESULT		0x24

#define D_IAP_BIN_PACKET_LEN            1024LL    // bin分包传输大小

// 回复上位机
typedef struct
{
    uint8_t cmd;

    uint8_t type;   // 0xFF-需要应答 0x80-回应的数据

    uint16_t parm1;
    uint16_t parm2;
}iap_tx_packet_rt;
iap_tx_packet_rt iap_tx_packet;


void MainWindow::data_send_proc(const QByteArray &data, uint8_t need_retry, uint16_t wait_time)
{
    current_sendData.resize(sizeof(iap_tx_packet));
    memcpy(current_sendData.data(), &iap_tx_packet, sizeof(iap_tx_packet));
    current_sendData.append(data);
    serialPortHandler->sendData(current_sendData);

    if (need_retry)
    {
        retryCount = 0;
        timeoutTimer->setInterval(wait_time);

        timeoutTimer->start(); // 启动定时器
    }
}


void MainWindow::onTimeout()
{
    if (retryCount < 3) {
        retryCount++;
        qDebug() << "未收到响应，重发数据，重发次数:" << retryCount;

        // 重发数据
        if (serialPortHandler->sendData(current_sendData)) {
            timeoutTimer->start(); // 重启定时器
        } else {
            QMessageBox::critical(this, "错误", "重发数据失败");
        }
    } else {
        qDebug() << "重发次数已达上限，停止发送";
        QMessageBox::critical(this, "错误", "重发次数已达上限，停止发送");
        timeoutTimer->stop();
    }
}

void MainWindow::onDataReceived(const QByteArray &data)
{
    timeoutTimer->stop();

    qDebug() << "接收到数据:" << data.toHex();

    typedef struct
    {
        uint8_t cmd;
        uint8_t err_code;
        uint16_t parm1;
        uint16_t parm2;
        uint8_t pbuf[256];
    }iap_packet_rt;
    iap_packet_rt iap_packet;   // 接收到的数据结构体

    typedef struct
    {
        uint16_t bin_index;
        uint16_t bin_size;
        uint16_t total_index;
    }curr_bin_packet_rt;
    static curr_bin_packet_rt curr_bin_packet;

    QByteArray chunk;

    memcpy(&iap_packet, data.constData(), data.size());

    qDebug("cmd: 0x%02x, err_code: 0x%02x, parm1: %d, parm2: %d.", iap_packet.cmd, iap_packet.err_code, iap_packet.parm1, iap_packet.parm2);
    switch(iap_packet.cmd)
    {
        case D_IAP_CMD_GET_DEVICE_INFO:     // 查询设备OK，握手成功
            ui->pushButton_connect_mcu->setText("断开连接");
            ui->pushButton_start_updata->setEnabled(true);
            break;

        case D_IAP_CMD_IAP_REQ:
            if (iap_packet.parm1*1024 <= fileSize)
            {
                QMessageBox::critical(this, "错误", "待升级固件太大");
            }
            else
            {
                iap_tx_packet.cmd = D_IAP_CMD_SEND_IAP_INFO;
                iap_tx_packet.type = 0xFF;
                iap_tx_packet.parm1 = D_IAP_BIN_PACKET_LEN;
                iap_tx_packet.parm2 = 0;

                data_send_proc(QByteArray(), 1, 8000);  // 等待设备擦除后的响应
            }
            break;

        case D_IAP_CMD_SEND_IAP_INFO:
            if (iap_packet.parm1 == D_IAP_BIN_PACKET_LEN)
            {
                // 启动传输
                bytesWritten = 0;
                curr_bin_packet.bin_index =0;
                curr_bin_packet.bin_size = qMin(D_IAP_BIN_PACKET_LEN, fileSize - bytesWritten);
                curr_bin_packet.total_index = (fileSize / D_IAP_BIN_PACKET_LEN + 1);

                chunk = fileData.mid(bytesWritten, curr_bin_packet.bin_size);

                iap_tx_packet.cmd = D_IAP_CMD_SEND_IAP_DATA;
                iap_tx_packet.type = 0xFF;
                iap_tx_packet.parm1 = curr_bin_packet.bin_index;
                iap_tx_packet.parm2 = curr_bin_packet.bin_size;

                data_send_proc(chunk, 1, 500);

                bytesWritten += curr_bin_packet.bin_size;
            }
            break;

        case D_IAP_CMD_SEND_IAP_DATA:
            qDebug("1: 0x%02x, 2: 0x%02x, 3: %d, 4: %d.", curr_bin_packet.bin_index, iap_packet.parm1, curr_bin_packet.bin_size, iap_packet.parm2);

            if (iap_packet.parm2 == 0)
            {
                // 收到空包回应
                qDebug("MCU全部写入完成回应");

                iap_tx_packet.cmd = D_IAP_CMD_GET_IAP_DATA_CRC;
                iap_tx_packet.type = 0xFF;
                iap_tx_packet.parm1 = 0;
                iap_tx_packet.parm2 = 0;

                data_send_proc(QByteArray(), 1, 500);
            }
            else if ((curr_bin_packet.bin_index == iap_packet.parm1) && (curr_bin_packet.bin_size == iap_packet.parm2))
            {
                ui->progressBar->setValue(bytesWritten);

                if (bytesWritten >= fileSize)
                {
                    qDebug("全部发送完成");

                    // 发送空包
                    curr_bin_packet.bin_index ++;

                    iap_tx_packet.cmd = D_IAP_CMD_SEND_IAP_DATA;
                    iap_tx_packet.type = 0xFF;
                    iap_tx_packet.parm1 = curr_bin_packet.bin_index;
                    iap_tx_packet.parm2 = 0;

                    data_send_proc(QByteArray(), 1, 500);
                }
                else
                {
                    curr_bin_packet.bin_index ++;
                    curr_bin_packet.bin_size = qMin(1024LL, fileSize - bytesWritten);

                    chunk = fileData.mid(bytesWritten, curr_bin_packet.bin_size);

                    iap_tx_packet.cmd = D_IAP_CMD_SEND_IAP_DATA;
                    iap_tx_packet.type = 0xFF;
                    iap_tx_packet.parm1 = curr_bin_packet.bin_index;
                    iap_tx_packet.parm2 = curr_bin_packet.bin_size;

                    data_send_proc(chunk, 1, 500);
                    bytesWritten += curr_bin_packet.bin_size;
                }
            }
            break;

        case D_IAP_CMD_GET_IAP_DATA_CRC:
            qDebug("rec: 0x%04x, 0x%04x", iap_packet.parm1, bin_crc);
            iap_tx_packet.cmd = D_IAP_CMD_SEND_CRC_RESULT;
            iap_tx_packet.type = 0xFF;
            iap_tx_packet.parm1 = (iap_packet.parm1 == bin_crc);
            iap_tx_packet.parm2 = 0;
            data_send_proc(QByteArray(), 1, 500);
            break;

        case D_IAP_CMD_SEND_CRC_RESULT:
            if (iap_packet.parm1 == 1)
            {
                on_pushButton_reset_mcu_clicked();
            }
            break;
    }
}

void MainWindow::onDataReceivedStr(const QByteArray &data)
{
    QString displayText;

    // 获取当前时间
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ");
    displayText += timestamp;

    for (char c : data)
    {
        // 可打印ASCII字符 (32-126)
        if (c >= 32 && c <= 126) {
            displayText += c;
        }
        // 处理特殊字符
        else {
            switch(c) {
                case '\n': displayText += "\\n"; break;
                case '\r': displayText += "\\r"; break;
                case '\t': displayText += "\\t"; break;
                default:
                    // 其他非打印字符显示为十六进制
                    displayText += QString("\\x%1").arg((quint8)c, 2, 16, QLatin1Char('0'));
            }
        }
    }

    ui->textEdit->append(displayText);
}




// 与MCU握手
void MainWindow::on_pushButton_connect_mcu_clicked()
{
    if (ui->pushButton_connect_mcu->text() == "断开连接")
    {
        // 断开连接
        on_pushButton_reset_mcu_clicked();
        ui->pushButton_start_updata->setEnabled(false);

        ui->pushButton_connect_mcu->setText("开始连接");
    }
    else
    {
        iap_tx_packet.cmd = D_IAP_CMD_GET_DEVICE_INFO;
        iap_tx_packet.type = 0xFF;
        iap_tx_packet.parm1 = 0;
        iap_tx_packet.parm2 = 0;

        data_send_proc(QByteArray(), 1, 200);
    }
}

void MainWindow::on_pushButton_start_updata_clicked()
{
    if (!fileSize)
    {
        QMessageBox::critical(this, "错误", "请选择bin文件");
        return;
    }

    // 启动升级流程不可再点
    ui->pushButton_start_updata->setEnabled(false);

    iap_tx_packet.cmd = D_IAP_CMD_IAP_REQ;
    iap_tx_packet.type = 0xFF;
    iap_tx_packet.parm1 = fileSize&0xFFFF;
    iap_tx_packet.parm2 = (fileSize>>8)&0xFFFF;

    data_send_proc(QByteArray(), 1, 200);
}


void MainWindow::on_pushButton_reset_mcu_clicked()
{
    iap_tx_packet.cmd = D_IAP_CMD_REBOOT;
    iap_tx_packet.type = 0xFF;
    iap_tx_packet.parm1 = 0;
    iap_tx_packet.parm2 = 0;

    data_send_proc(QByteArray(), 0, 200);
}


void MainWindow::on_pushButton_reset_mcu_2_clicked()
{
    // 给定的数组
    unsigned char data[] = {0x55, 0xDB, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x0A, 0xB7, 0x29, 0x0D, 0x0A};

    // 计算数组的长度
    int length = sizeof(data) / sizeof(data[0]);

    // 将数组转换为 QByteArray
    QByteArray byteArray(reinterpret_cast<const char*>(data), length);
//    data_send_proc(byteArray, 0, 200);
    qDebug() << "QByteArray content will send hex: " << byteArray.toHex().toUpper();
    serialPortHandler->serialPort->write(byteArray);
}

void MainWindow::on_pushButton_set_unixtime_clicked()
{
//    iap_tx_packet.cmd = D_IAP_CMD_GET_DEVICE_INFO;
//    iap_tx_packet.type = 0xFF;
//    iap_tx_packet.parm1 = 0;
//    iap_tx_packet.parm2 = 0;

//    data_send_proc(QByteArray(), 0, 200);
}

void MainWindow::on_pushButton_calculate_coeff_clicked()
{
    if (ui->lineEdit_pulse_voltage->text().isEmpty())
    {
        QMessageBox::critical(this, "错误", "请输入实测脉冲电压值");
    }

    QString text = ui->lineEdit_pulse_voltage->text();
    bool conver_ok;
    float pulse_voltage = text.toFloat(&conver_ok);

    if ((pulse_voltage > 3.0)|| (pulse_voltage < 1.0))
    {
        QMessageBox::critical(this, "错误", "请输入正确的脉冲电压值");
    }
    else if (conver_ok)
    {
        // 转换OK
        float coeff = 2.0/pulse_voltage;
        ui->lineEdit_coeff->setText(QString::number(coeff, 'f', 3));
    }
    else
    {
        // 转换失败，显示错误提示
        QMessageBox::critical(this, "错误", "请输入有效的脉冲电压值");
    }
}

void MainWindow::on_pushButton_set_calib_clicked()
{
    if (ui->lineEdit_coeff->text().isEmpty())
    {
        QMessageBox::critical(this, "错误", "请先计算coeff系数");
    }
    else if (!serialPortHandler->isPortOpen())
    {
        QMessageBox::critical(this, "错误", "请先打开串口");
    }
    else
    {
        // 给定的数组
        unsigned char data[] = {0x55, 0xDB, 0x01, 0x00, 0x01, 0x01, 0xFF, 0xFF, 0x0C, 0xCC, 0xCC, 0x0D, 0x0A};

        uint16_t temp;
        temp = ui->lineEdit_coeff->text().toFloat() * 1000;
        data[6] = (temp >> 8);
        data[7] = (temp);

        data[9] = (serialPortHandler->dataProtocol->crc16_modbus(data, 9) >> 8);
        data[10] = (serialPortHandler->dataProtocol->crc16_modbus(data, 9) );

        // 计算数组的长度
        int length = sizeof(data) / sizeof(data[0]);

        // 将数组转换为 QByteArray
        QByteArray byteArray(reinterpret_cast<const char*>(data), length);
        qDebug() << "QByteArray content will send hex: " << byteArray.toHex().toUpper();
        serialPortHandler->serialPort->write(byteArray);
    }
}


void MainWindow::on_pushButton_read_calib_clicked()
{
    if (serialPortHandler->isPortOpen())
    {
        // 给定的数组
        unsigned char data[] = {0x55, 0xDB, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x0B, 0x77, 0xE8, 0x0D, 0x0A};

        // 计算数组的长度
        int length = sizeof(data) / sizeof(data[0]);

        // 将数组转换为 QByteArray
        QByteArray byteArray(reinterpret_cast<const char*>(data), length);
        qDebug() << "QByteArray content will send hex: " << byteArray.toHex().toUpper();
        serialPortHandler->serialPort->write(byteArray);
    }
    else
    {
        QMessageBox::critical(this, "错误", "请先打开串口");
    }
}

void MainWindow::on_pushButton_log_clear_clicked()
{
    ui->textEdit->clear();
}
