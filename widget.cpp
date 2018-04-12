#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    mode = "socket";

    /// SOCKET SETUP
    socket = new QTcpSocket(this);

    connect(socket, SIGNAL(connected()), this, SLOT(sckConnected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(sckDisconnected()));
    connect(socket, SIGNAL(bytesWritten(qint64)), this, SLOT(sckBytesWritten(qint64)));
    connect(socket, SIGNAL(readyRead()), this, SLOT(sckReadyRead()));
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(skcStateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(sckError(QAbstractSocket::SocketError)));

    states_str    << "unconnected" << "host lookup" << "connecting" << "connected" << "bound" << "closing" << "listening";
    states_colors << "red"         << "orange"      << "orange"     << "green"     << "green" << "orange"  << "orange";

    /// SERIAL SETUP
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->comboBox_serial_available->addItem(info.portName());
    }

    QStringList baudlist;
    baudlist << "1200" << "2400" << "4800" << "9600" << "19200" << "38400" << "57600" << "115200" << "625000" << "921600";
    ui->comboBox_serial_baud->addItems(baudlist);
    ui->comboBox_serial_baud->setCurrentIndex(7);

    // serial data available signal
    connect(&serialport, SIGNAL(readyRead()), this, SLOT(serialReadyRead()));

    /// UI SIGNAL CONNECTION
    connect(ui->pushButton_connect,    SIGNAL(clicked(bool)), this, SLOT(sckConnect()));
    connect(ui->pushButton_disconnect, SIGNAL(clicked(bool)), this, SLOT(sckDisconnect()));
    connect(ui->pushButton_cmdSend,    SIGNAL(clicked(bool)), this, SLOT(on_lineEdit_cmdSend_returnPressed()));

    ui->label_status->setText("State: socket " + states_str[socket->state()]);
    ui->label_status->setStyleSheet("QLabel {color : " + states_colors[socket->state()] + "; }");

    QList<int> sizes;
    sizes << 150 << 450;
    ui->splitter->setSizes(sizes);

    hist_len = 0;
    hist_idx = 0;

    ui->tabWidget->setCurrentIndex(0);
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(plotSelected(int)));

    int width  = 700;
    int height = 700;
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    this->setGeometry(screenGeometry.width()/2 - width/2, screenGeometry.height()/2 - height/2, width, height);

    qApp->installEventFilter(this);

    ui->tabWidget_2->setCurrentIndex(0);

    /// PLOT SETUP
    timerPlot = new QTimer(this);
    connect(timerPlot, SIGNAL(timeout()), this, SLOT(plotUpdate()));

//    isFirst = true;
//    validData = false;

    ui->dockWidget->setFloating(true);
    ui->dockWidget->hide();
    QRect dockGeometry = ui->dockWidget->geometry();
    QRect thisGeometry = this->geometry();
    ui->dockWidget->setGeometry(windowPos.x() - dockGeometry.width() - 20, windowPos.y(), dockGeometry.width(), thisGeometry.height());

    ui->plot->plotLayout()->clear();
    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes | QCP::iMultiSelect);

    addSubplotTab();

    qDebug() << "sizeof(dataType) = " << sizeof(data_u.data);
    qDebug() << "sizeof(int) = " << sizeof(int);
    qDebug() << "sizeof(unsigned int) = " << sizeof(unsigned int);
    qDebug() << "sizeof(float) = " << sizeof(float);
    qDebug() << "sizeof(double) = " << sizeof(double);
    qDebug() << "sizeof(int16_t) = " << sizeof(int16_t);

    parsedData = QVector<QVector<double>>(numData);

    // LINK X AXES
    ui->checkBox_xWindowLink->setChecked(true);
    ui->horizontalSlider_xWindowLink->setValue(500);

    last_packet_id = -1;
}

Widget::~Widget()
{
    sckDisconnect();
    delete ui;
}
///////////////////////////////////////////////////////////////////////////// COBS DECODE
int MyCOBSdecode(QByteArray data_byte_encode, size_t packetlen) {
//  size_t datalen_encode = data_byte_encode.length();
  size_t read_index = 0;
  size_t write_index = 0;
  uint8_t code;
  uint8_t i;
  while(read_index < packetlen) {
//    code = data_byte_encode[read_index];
    code = data_byte_encode.at(read_index);
    if(read_index + code > packetlen && code != 1) {
      return 0;
    }
    read_index++;
    for(i = 1; i < code; i++) {
        data_u.data_byte[write_index++] = data_byte_encode.at(read_index++);
    }
    if(code != 0xFF && read_index != packetlen) {
        data_u.data_byte[write_index++] = '\0';
    }
  }
  return write_index;
}
///////////////////////////////////////////////////////////////////////////// SOKET SLOTS
void Widget::sckConnect() {

    QString hostName = ui->lineEdit_hostname->text();
    int port = ui->lineEdit_port->text().toInt();


    qDebug() << "connecting...";

    // this is not blocking call
    socket->connectToHost(hostName, port);

    // we need to wait...
    if(!socket->waitForConnected(5000))
    {
        qDebug() << "Error: " << socket->errorString();
        QMessageBox::critical(this, tr("ESP8266 comunication tool"), (socket->errorString()), QMessageBox::Ok);
    }
}

void Widget::sckConnected() {
    qDebug() << "connected...";
}

void Widget::sckDisconnect() {
    if(socket) {
        if(socket->state() == QTcpSocket::ConnectedState){
            qDebug() << "disconnecting...";
            socket->disconnectFromHost();
        } else {
            qDebug() << "was already disconnected...";
        }
    }
}

void Widget::sckDisconnected() {
    qDebug() << "disconnected...";
}

void Widget::sckBytesWritten(qint64 bytes) {
    qDebug() << "sended " << bytes << " bytes";
}

void Widget::sckReadyRead() {
    QByteArray incomingdata = socket->readAll();
//    processIncomingData(incomingdata);
    incomingdata_buffered.append(incomingdata);
    processIncomingData2();
}

void Widget::sckSend(QString tosend) {
    const char *tosendAscii = tosend.toStdString().c_str();
    socket->write(tosendAscii);
}

void Widget::skcStateChanged(QAbstractSocket::SocketState socketState) {
    qDebug() << "socket state changed: " << states_str[socketState];
    ui->label_status->setText("Status: " + states_str[socketState]);
    ui->label_status->setStyleSheet("QLabel {color : " + states_colors[socketState] + "; }");
    QApplication::processEvents();
    QApplication::processEvents();
}

void Widget::sckError(QAbstractSocket::SocketError socketError) {
    qDebug() << "Error signal " << socketError;
}

//////////////////////////////////////////////////////////////////////////


void Widget::on_lineEdit_cmdSend_returnPressed()
{
    ui->listWidget_cmdHistory->clearSelection();
    if(mode == "socket") {
        if(socket->state() == QTcpSocket::ConnectedState) {
            QString cmd = ui->lineEdit_cmdSend->text();
            if(cmd != "") {
                sckSend(cmd);
                ui->listWidget_cmdHistory->addItem(cmd);
                ui->listWidget_cmdHistory->scrollToBottom();
                ui->lineEdit_cmdSend->clear();
            } else {
                qDebug() << "you can not send empty cmd...";
                QMessageBox::warning(this, tr("ESP8266 comunication tool"), tr("You can not send empty command."), QMessageBox::Ok);
            }
        } else {
            qDebug() << "you are not connected...";
            QMessageBox::warning(this, tr("comunication tool"), tr("You can not send command.\nYou are not connected to host."), QMessageBox::Ok);
        }
    } else if(mode == "serial") {
        if(serialport.isOpen()) {
            QString cmd = ui->lineEdit_cmdSend->text();
            if(cmd != "") {
                const char* tosend = cmd.toStdString().c_str();
                serialport.write(tosend);
                ui->listWidget_cmdHistory->addItem(cmd);
                ui->listWidget_cmdHistory->scrollToBottom();
                ui->lineEdit_cmdSend->clear();
            } else {
                qDebug() << "you can not send empty cmd...";
                QMessageBox::warning(this, tr("comunication tool"), tr("You can not send empty command."), QMessageBox::Ok);
            }
        } else {
            qDebug() << "serial port is closed...";
            QMessageBox::warning(this, tr("comunication tool"), tr("You can not send command.\nSerial port is not opened."), QMessageBox::Ok);
        }

    }
}

bool Widget::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->lineEdit_cmdSend && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down) {
            hist_len = ui->listWidget_cmdHistory->count();
            if(ui->listWidget_cmdHistory->selectedItems().isEmpty()) {
                hist_idx = hist_len - 1;
            } else if (keyEvent->key() == Qt::Key_Up) {
                hist_idx--;
            } else {
                hist_idx++;
            }
            hist_idx = hist_idx == -1 ? hist_len - 1 : hist_idx;
            hist_idx = hist_idx == hist_len ? 0 : hist_idx;
            ui->listWidget_cmdHistory->setCurrentRow(hist_idx);
            ui->lineEdit_cmdSend->setText(ui->listWidget_cmdHistory->selectedItems().first()->text());
            return true;
        }

    }
     return QWidget::eventFilter(obj, event);
}

void Widget::on_pushButton_saveIncomingData_clicked()
{
    QString filename = QFileDialog::getSaveFileName( this, tr("Save Recieved Data"), QDir::rootPath(), tr("CSV files (*.csv)")  );
    if (filename != "") {
        qDebug() << filename;
        QFile f( filename );
        if (f.open( QIODevice::WriteOnly ) ) {
            QTextStream stream( &f );
            stream << ui->textBrowser_incomingData->toPlainText();
            f.close();
        }
    } else {
        qDebug() << "non è stato sel alcun file";
    }
}

void Widget::on_pushButton_plotSettings_clicked()
{
    if(ui->dockWidget->isVisible()) {
        ui->dockWidget->hide();
    } else {
        ui->dockWidget->show();
        QRect dockGeometry = ui->dockWidget->geometry();
        QRect thisGeometry = this->geometry();
        ui->dockWidget->setGeometry(windowPos.x() - dockGeometry.width() - 20, windowPos.y(), dockGeometry.width(), thisGeometry.height());
    }

}

void Widget::moveEvent(QMoveEvent *event){
    windowPos = event->pos();
}

void Widget::plotSelected(int tab_idx) {
    if(tab_idx == 1) {
        ui->dockWidget->show();
        QRect dockGeometry = ui->dockWidget->geometry();
        QRect thisGeometry = this->geometry();
        ui->dockWidget->setGeometry(windowPos.x() - dockGeometry.width() - 20, windowPos.y(), dockGeometry.width(), thisGeometry.height());
    }
}

void Widget::on_checkBox_enablePlot_toggled(bool checked)
{
    if(checked) {
        timerPlot->start(1000/ui->horizontalSlider_fps->value());
    } else {
        timerPlot->stop();
    }
}

void Widget::plotUpdate() {
    if(!parsedData[0].isEmpty()){
        emit(newDataAvailable(parsedData));
        ui->plot->replot();
        for(int k = 0; k<numData; k++) {
            parsedData[k].clear();
        }
    }
}

void Widget::on_horizontalSlider_fps_sliderMoved(int position) {
    timerPlot->setInterval(1000/position);
}

void Widget::on_spinBox_numberSubplots_valueChanged(int arg1) {
    int ntabs = ui->tabWidget_subplot->count();
    if(ntabs < arg1) {
        for(int i = ntabs; i < arg1; i++) {
            addSubplotTab();
        }
    } else {
        for(int i = ntabs; i > arg1; i--) {
            removeSubplotTab();
        }
    }
}

void Widget::linePropertiesChanged(linePropertiesT lineProp) {
    // ok riesco a connettere slot del padre a segnali del figlio
//    SubplotTab* prova = (SubplotTab*)QObject::sender()->parent();
//    qDebug() << currentTab->findChild<QLabel*>("label_numberLines");
    qDebug() << "Line Property changed:";
    qDebug() << "   id: " << lineProp.id;
    qDebug() << "   xIndex: " << lineProp.xIndex;
    qDebug() << "   yIndex: " << lineProp.yIndex;

}

void Widget::addSubplotTab() {
    int tabNumber = ui->tabWidget_subplot->count() + 1;
    SubplotTab* newTab = new SubplotTab(this, tabNumber, ui->plot);
    connect(ui->checkBox_xWindowLink, SIGNAL(toggled(bool)), newTab, SLOT(link_x_windows_check_changed(bool)));
    connect(ui->horizontalSlider_xWindowLink, SIGNAL(valueChanged(int)), newTab, SLOT(link_x_windows_value_changed(int)));
    newTab->link_x_windows_check_changed(ui->checkBox_xWindowLink->isChecked());
    ui->tabWidget_subplot->addTab(newTab, "Subplot " + QString::number(tabNumber));
    ui->tabWidget_subplot->setCurrentWidget(newTab);
}

void Widget::removeSubplotTab() {
    int toRemove = ui->tabWidget_subplot->count() - 1;
    // PREFERIVO METTERE QUESTA PARTE NEL DISTRUTTORE DELLA TAB MA CRASHA AL MOMENTO DELLA CHIUSURA DELLA UI
    ui->plot->plotLayout()->removeAt(toRemove);
    ui->plot->plotLayout()->simplify();
    ui->plot->replot();
    //
    delete ui->tabWidget_subplot->widget(toRemove);
    ui->tabWidget_subplot->removeTab(toRemove);
}

void Widget::on_tabWidget_subplot_currentChanged(int index)
{
    currentTabIdx = index;
    currentTab    = (SubplotTab*)ui->tabWidget_subplot->widget(index);
    qDebug() << "Current tab: " << index;
}

void Widget::on_pushButton_clearPlots_clicked()
{
//    qDebug() << "Widget::on_pushButton_clearPlots_clicked()";
    int count = ui->tabWidget_subplot->count();
//    qDebug() << "count = " << count;
    for(int i = 0; i < count; i++){
        SubplotTab* toclear = (SubplotTab*)ui->tabWidget_subplot->widget(i);
//        qDebug() << toclear;
        toclear->clearPlot();
    }
    ui->plot->replot();
}

/// SERIAL

void Widget::serialReadyRead() {
    QByteArray incomingdata = serialport.readAll();
    qDebug() << "incomingdata.length() = " << incomingdata.length();
    qDebug() << "incomingdata = " << incomingdata.toHex();
    //    processIncomingData(incomingdata);
    incomingdata_buffered.append(incomingdata);
    processIncomingData2();
}

void Widget::on_comboBox_serial_available_currentIndexChanged(const QString &arg1)
{
    serialportname = arg1;
    qDebug() << "serial port: " << serialportname;
    QSerialPortInfo serialportinfo(serialportname);
    QString serialinfos = QString("<html><b>Manufacturer:</b>\n%1\n<b>Description:</b>\n%2</html>").arg(serialportinfo.manufacturer(), serialportinfo.description());
    ui->textBrowser_serial_info->setText(serialinfos);
}

void Widget::on_comboBox_serial_baud_currentIndexChanged(const QString &arg1)
{
    serialbaud = arg1;
}

void Widget::on_pushButton_serial_start_clicked()
{
    if(serialport.isOpen()) {
        QMessageBox::warning(this, tr("comunication tool"), tr("Serial port was already open."), QMessageBox::Ok);
        return;
    }

    QSerialPortInfo serialportinfo(serialportname);
    if (serialportinfo.isBusy()) {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","The selected serial port is busy");
        messageBox.setFixedSize(500,200);
        return;
    }
    if (serialportname.isEmpty()) {
        QMessageBox messageBox;
        messageBox.warning(0,"Warning","No serial port selected");
        messageBox.setFixedSize(500,200);
        return;
    }
    serialport.setPortName(serialportname);
    serialport.open(QIODevice::ReadWrite);
    serialport.setBaudRate( serialbaud.toInt() );
    serialport.setDataBits(QSerialPort::Data8);
    serialport.setParity(QSerialPort::NoParity);
    serialport.setStopBits(QSerialPort::OneStop);
    serialport.setFlowControl(QSerialPort::NoFlowControl);

    QString serialportstate = serialport.isOpen()?"open":"closed";
    QString color = serialport.isOpen()?"green":"red";
    ui->label_status->setText("State: serial port " + serialportstate);
    ui->label_status->setStyleSheet("QLabel {color : "+ color + "; }");
}

void Widget::on_pushButton_serial_stop_clicked()
{
    if(serialport.isOpen()) {
        serialport.close();
        QString serialportstate = serialport.isOpen()?"open":"closed";
        QString color = serialport.isOpen()?"green":"red";
        ui->label_status->setText("State: serial port " + serialportstate);
        ui->label_status->setStyleSheet("QLabel {color : "+ color + "; }");
    } else {
        qDebug() << "serial port is closed...";
        QMessageBox::warning(this, tr("comunication tool"), tr("Serial port was already closed."), QMessageBox::Ok);
    }
}

void Widget::on_tabWidget_2_currentChanged(int index)
{
    if(index == 0) {
        mode = "socket";
        ui->label_status->setText("State: socket " + states_str[socket->state()]);
        ui->label_status->setStyleSheet("QLabel {color : " + states_colors[socket->state()] + "; }");
    } else if(index == 1) {
        mode = "serial";
        QString serialportstate = serialport.isOpen()?"open":"closed";
        QString color = serialport.isOpen()?"green":"red";
        ui->label_status->setText("State: serial port " + serialportstate);
        ui->label_status->setStyleSheet("QLabel {color : "+ color + "; }");
    }
    qDebug() << "MODE = " << mode;
}

//

void Widget::processIncomingData(QByteArray incoming) {
    const int old_scrollbar_value = ui->textBrowser_incomingData->verticalScrollBar()->value();

        qDebug() << "incoming.length() = " << incoming.length();
        qDebug() << "incoming = " << incoming.toHex();
        QList<QByteArray> intest_splitted = incoming.split(0);
        qDebug() << "intest_splitted.length() = " << intest_splitted.length();
        QListIterator<QByteArray> packet_it(intest_splitted);
        while (packet_it.hasNext()) {
            QByteArray packet = packet_it.next();
//            qDebug() << "packet length = " << packet.length();
            if(!packet.isEmpty() && packet.length() == sizeof(dataType)+1) {
                MyCOBSdecode(packet, sizeof(dataType)+1);
    //            QString data = "time = " + QString::number(data_u.data.time) + ",\tdata1 = " + QString::number(data_u.data.data1) + ",\tdata2 = " + QString::number(data_u.data.data2) + ",\tdata3 = " + QString::number(data_u.data.data3) + ",\tdata4 = " + QString::number(data_u.data.data4) + "\n";
    //            QString data = QString::number(data_u.data.time) + "\t" + QString::number(data_u.data.data1) + "\t" + QString::number(data_u.data.data2) + "\t" + QString::number(data_u.data.data3) + "\t" + QString::number(data_u.data.data4) + "\n";
                QString data = QString::number(data_u.data.pck_n)  + ",\t" +
                               QString::number(data_u.data.time)  + ",\t" +
                               QString::number(data_u.data.data1) + ",\t" +
                               QString::number(data_u.data.data2) + ",\t" +
                               QString::number(data_u.data.data3) + ",\t" +
                               QString::number(data_u.data.data4) + ",\t" +
                               QString::number(data_u.data.data5) + ",\t" +
                               QString::number(data_u.data.data6) + "\n"; // ",\t" +
//                               QString::number(data_u.data.data7) + ",\t" +
//                               QString::number(data_u.data.data8) + ",\t" +
//                               QString::number(data_u.data.data9) + "\n";
                ui->textBrowser_incomingData->moveCursor(QTextCursor::End);
                ui->textBrowser_incomingData->insertPlainText(data);

                // populate plot data
                parsedData[0].append(double(data_u.data.time));
                parsedData[1].append(double(data_u.data.data1));
                parsedData[2].append(double(data_u.data.data2));
                parsedData[3].append(double(data_u.data.data3));
                parsedData[4].append(double(data_u.data.data4));
                parsedData[5].append(double(data_u.data.data5));
                parsedData[6].append(double(data_u.data.data6));
//                parsedData[7].append(data_u.data.data7);
//                parsedData[8].append(data_u.data.data8);
//                parsedData[9].append(data_u.data.data9);

            } else {
                qDebug() << "packet.length() = " << packet.length();
                qDebug() << "packet =   " << packet.toHex();
            }
        }

        if(!ui->checkBox_autoscroll->isChecked()) {
            ui->textBrowser_incomingData->verticalScrollBar()->setValue(old_scrollbar_value);
        }
}

void Widget::processIncomingData2() {
    const int old_scrollbar_value = ui->textBrowser_incomingData->verticalScrollBar()->value();

//    qDebug() << "incoming_buf.length() before processing = " << incomingdata_buffered.length();
//    qDebug() << "incoming_buf = " << incomingdata_buffered.toHex();
    QList<QByteArray> incomingdata_buffered_splitted = incomingdata_buffered.split(0);
    incomingdata_buffered_splitted.removeLast(); // the last is empty
    //        qDebug() << "intest_splitted.length() = " << incomingdata_buffered_splitted.length();
    //        qDebug() << "intest_splitted.at(last) = " << incomingdata_buffered_splitted.at(incomingdata_buffered_splitted.length()-1).toHex();
    QListIterator<QByteArray> packet_it(incomingdata_buffered_splitted);
    while (packet_it.hasNext()) {
        QByteArray packet = packet_it.next();
        if(packet.length() == sizeof(dataType)+1) {
            incomingdata_buffered.remove(0, sizeof(dataType)+2);
            MyCOBSdecode(packet, sizeof(dataType)+1);
//            qDebug() << "packet processed";
            QString data = QString::number(data_u.data.pck_n)  + ",\t" +
                    QString::number(data_u.data.time)  + ",\t" +
                    QString::number(data_u.data.data1) + ",\t" +
                    QString::number(data_u.data.data2) + ",\t" +
                    QString::number(data_u.data.data3) + ",\t" +
                    QString::number(data_u.data.data4) + ",\t" +
                    QString::number(data_u.data.data5) + ",\t" +
                    QString::number(data_u.data.data6) + "\n"; // ",\t" +

                    ui->textBrowser_incomingData->moveCursor(QTextCursor::End);
                    ui->textBrowser_incomingData->insertPlainText(data);

                    // populate plot data
                    parsedData[0].append(double(data_u.data.time));
                    parsedData[1].append(double(data_u.data.data1));
                    parsedData[2].append(double(data_u.data.data2));
                    parsedData[3].append(double(data_u.data.data3));
                    parsedData[4].append(double(data_u.data.data4));
                    parsedData[5].append(double(data_u.data.data5));
                    parsedData[6].append(double(data_u.data.data6));

//                    if(last_packet_id != -1){
//                        if(data_u.data.pck_n - last_packet_id  - 1) qDebug() << data_u.data.pck_n - last_packet_id - 1 << " PACKET LOSS";
//                    }
                    last_packet_id = data_u.data.pck_n;
        }
    }
//    if(incomingdata_buffered.length()) qDebug() << "incoming_buf.length() after processing = " << incomingdata_buffered.length();

        if(!ui->checkBox_autoscroll->isChecked()) {
            ui->textBrowser_incomingData->verticalScrollBar()->setValue(old_scrollbar_value);
        }
}


void Widget::on_pushButton_clear_clicked()
{
    ui->textBrowser_incomingData->clear();
    ui->plot->update();
}

void Widget::on_pushButton_refresh_serial_clicked()
{
    ui->comboBox_serial_available->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->comboBox_serial_available->addItem(info.portName());
    }
}

void Widget::on_checkBox_xWindowLink_toggled(bool checked)
{
    ui->horizontalSlider_xWindowLink->setEnabled(checked);
}

void Widget::on_horizontalSlider_xWindowLink_valueChanged(int value)
{
    ui->label_xWindowLink->setText(QString::number(value/100.0, 'f', 2) + "s");
}
