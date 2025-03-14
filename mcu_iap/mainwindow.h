#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "serialporthandler.h"

#include <QSettings>
#include <QFile>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    // 配置文件对象
    QSettings *settings;

    void save_windows_parm();
    void read_windows_parm();

private slots:
    void onTimeout(); // 定时器超时槽函数

    void onDataReceived(const QByteArray &data);
    void handleSerialPortError(const QString &error);

    void data_send_proc(const QByteArray &data, uint8_t need_retry, uint16_t wait_time);

    void on_pushButton_refresh_clicked();

    void on_pushButton_open_clicked();

    void on_pushButton_select_file_clicked();

    void on_pushButton_connect_mcu_clicked();

    void on_pushButton_reset_mcu_clicked();

    void on_pushButton_start_updata_clicked();

    void on_pushButton_reset_mcu_2_clicked();

    void on_pushButton_set_unixtime_clicked();

private:
    Ui::MainWindow *ui;
    SerialPortHandler *serialPortHandler;

    QTimer *timeoutTimer;
    uint16_t retryCount; // 重发次数
    uint16_t wait_time_cnt;  // 等待时间

    // 当前串口发送的数据
    QByteArray current_sendData;

    // bin文件读取
    QFile *file;
    uint32_t fileSize;
    uint16_t bin_crc;
    qint64 bytesWritten;
    QByteArray fileData;  // 文件数据缓存
};

#endif // MAINWINDOW_H
