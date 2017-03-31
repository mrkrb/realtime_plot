#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDesktopWidget>
#include <QRect>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QDebug>
#include <QKeyEvent>
#include <QList>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTimer>
#include <QStringList>
#include <QtSerialPort>

#include "subplottab.h"
#include "lineform.h"
#include "qcustomplot/qcustomplot.h"

//#define TEST

namespace Ui {
class Widget;
}

//struct __attribute__((packed)) dataType{
//  float time;
//  int16_t data1;
//  float data2;
//  int16_t data3;
//  float data4;
//};

struct __attribute__((packed)) dataType{
  float time;
  float data1;
  float data2;
  float data3;
  float data4;
  float data5;
  float data6;
  float data7;
  float data8;
  float data9;
};


union {
    dataType data;
    unsigned char data_byte[sizeof(dataType)];
  } data_u;

int MyCOBSdecode(QByteArray data_byte_encode, int packetlen);

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private:
    QString mode;

    Ui::Widget *ui;

    QTcpSocket *socket;

    void sckSend(QString tosend);
    int hist_len;
    int hist_idx;

    QList<QString> states_str;
    QList<QString> states_colors;

    QPoint windowPos;
    int plotSettingsWidth = 300;

    QTimer* timerPlot;

    QString initChar;
    QString plotData;
    bool isFirst;
    bool validData;
    int numData = 10; // COBS NUMBER OF FIELDS!!!
    QVector<QVector<double>> parsedData;

    void addSubplotTab();
    void removeSubplotTab();
    int currentTabIdx;
    SubplotTab* currentTab;

    ///SERIAL
    QString serialportname;
    QString serialbaud;
    QSerialPort serialport;

#ifdef TEST
    QTime timerTest;
#endif

protected:
    bool eventFilter(QObject *obj, QEvent *event);

    void moveEvent(QMoveEvent *event);

private slots:
    void sckConnect();
    void sckConnected();
    void sckDisconnect();
    void sckDisconnected();
    void sckBytesWritten(qint64 bytes);
    void sckReadyRead();
    void skcStateChanged(QAbstractSocket::SocketState socketState);
    void sckError(QAbstractSocket::SocketError socketError);

    void serialReadyRead();

    void on_lineEdit_cmdSend_returnPressed();
    void on_pushButton_saveIncomingData_clicked();
    void on_pushButton_plotSettings_clicked();
    void plotSelected(int);

    void plotUpdate();
    void on_checkBox_enablePlot_toggled(bool checked);
    void on_horizontalSlider_fps_sliderMoved(int position);
    void on_spinBox_numberSubplots_valueChanged(int arg1);

    void linePropertiesChanged(linePropertiesT);
    void on_tabWidget_subplot_currentChanged(int index);

    void on_comboBox_serial_available_currentIndexChanged(const QString &arg1);
    void on_comboBox_serial_baud_currentIndexChanged(const QString &arg1);
    void on_pushButton_serial_start_clicked();
    void on_pushButton_serial_stop_clicked();
    void on_tabWidget_2_currentChanged(int index);

    void processIncomingData(QByteArray incoming);

signals:
    void newDataAvailable(QVector<QVector<double>>const &);
};

#endif // WIDGET_H
