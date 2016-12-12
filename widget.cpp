#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    // SOCKET SETUP
    socket = new QTcpSocket(this);

    connect(socket, SIGNAL(connected()), this, SLOT(sckConnected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(sckDisconnected()));
    connect(socket, SIGNAL(bytesWritten(qint64)), this, SLOT(sckBytesWritten(qint64)));
    connect(socket, SIGNAL(readyRead()), this, SLOT(sckReadyRead()));
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(skcStateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(sckError(QAbstractSocket::SocketError)));

    states_str    << "Unconnected" << "Host Lookup" << "Connecting" << "Connected" << "Bound" << "Closing" << "Listening";
    states_colors << "red"         << "orange"      << "orange"     << "green"     << "green" << "orange"  << "orange";

    // UI SIGNAL CONNECTION

    connect(ui->pushButton_connect,    SIGNAL(clicked(bool)), this, SLOT(sckConnect()));
    connect(ui->pushButton_disconnect, SIGNAL(clicked(bool)), this, SLOT(sckDisconnect()));
    connect(ui->pushButton_cmdSend,    SIGNAL(clicked(bool)), this, SLOT(on_lineEdit_cmdSend_returnPressed()));

    ui->label_status->setText("State: " + states_str[socket->state()]);
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

    // PLOT SETUP
    timerPlot = new QTimer(this);
    connect(timerPlot, SIGNAL(timeout()), this, SLOT(plotUpdate()));

    initChar = ui->lineEdit_initChar->text();
    isFirst = true;
    validData = false;
    numData = 0;

    ui->dockWidget->setFloating(true);
    ui->dockWidget->hide();
    QRect dockGeometry = ui->dockWidget->geometry();
    QRect thisGeometry = this->geometry();
    ui->dockWidget->setGeometry(windowPos.x() - dockGeometry.width() - 20, windowPos.y(), dockGeometry.width(), thisGeometry.height());

    ui->plot->plotLayout()->clear();
    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes | QCP::iMultiSelect);

    addSubplotTab();

    qDebug() << "sizeof(dataType) = " << sizeof(data_u.data);
    qDebug() << "sizeof(float) = " << sizeof(float);
    qDebug() << "sizeof(int16_t) = " << sizeof(int16_t);

}

Widget::~Widget()
{
    sckDisconnect();
    delete ui;
}
///////////////////////////////////////////////////////////////////////////// COBS DECODE
int MyCOBSdecode(QByteArray data_byte_encode, int packetlen) {
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
    const int old_scrollbar_value = ui->textBrowser_incomingData->verticalScrollBar()->value();

//    QString incoming = socket->readAll();
//    QString incoming = "sckReadyRead\n";

    QByteArray intest = socket->readAll();
    QList<QByteArray> intest_splitted = intest.split(0);
    QListIterator<QByteArray> packet_it(intest_splitted);
    while (packet_it.hasNext()) {
        QByteArray packet = packet_it.next();
        if(!packet.isEmpty()) {
            MyCOBSdecode(packet, 17);
//            QString data = "time = " + QString::number(data_u.data.time) + ",\tdata1 = " + QString::number(data_u.data.data1) + ",\tdata2 = " + QString::number(data_u.data.data2) + ",\tdata3 = " + QString::number(data_u.data.data3) + ",\tdata4 = " + QString::number(data_u.data.data4) + "\n";
            QString data = QString::number(data_u.data.time) + "\t" + QString::number(data_u.data.data1) + "\t" + QString::number(data_u.data.data2) + "\t" + QString::number(data_u.data.data3) + "\t" + QString::number(data_u.data.data4) + "\n";
            ui->textBrowser_incomingData->moveCursor(QTextCursor::End);
            ui->textBrowser_incomingData->insertPlainText(data);
        }
    }

    if(ui->checkBox_enablePlot->isChecked()) {
//        plotData.append(incoming);
    }

    if(!ui->checkBox_autoscroll->isChecked()) {
        ui->textBrowser_incomingData->verticalScrollBar()->setValue(old_scrollbar_value);
    }
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
        QMessageBox::warning(this, tr("ESP8266 comunication tool"), tr("You can not send command.\nYou are not connected to host."), QMessageBox::Ok);
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
    QString filename = QFileDialog::getSaveFileName( this, tr("Save Recieved Data"), QDir::rootPath(), tr("Text files (*.txt)")  );
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
//    qDebug() << "update";

    if(!plotData.isEmpty() && socket->state() == QTcpSocket::ConnectedState){

        QStringList plotDataLines = plotData.split("\r\n");

        foreach(const QString &line, plotDataLines){
            if( line.startsWith(initChar) ) {
                validData = true;
                QStringList plotDataParsed = line.split(",");

                if(isFirst) {
                    numData = plotDataParsed.length();
                    parsedData = QVector<QVector<double>>(numData);
                    isFirst = false;
                }

                int i = 0;
                foreach (QString data, plotDataParsed) {
                    parsedData[i] << data.remove(0,1).toFloat();
                    i++;
                }
            }
        }




        if(validData) {
            // EMIT THE SIGNAL FOR NEW DATA
            emit(newDataAvailable(parsedData));
            ui->plot->replot();

            // EMPTY THE DATA BUFFER
            for(int k = 0; k<numData; k++) {
                parsedData[k].clear();
            }
        }

        plotData.clear();
        validData = false;
    }

#ifdef TEST
    if(isFirst) {
        numData = 5;
        parsedData = QVector<QVector<double>>(numData);
        timerTest.start();
        isFirst = false;
    }
    double time = timerTest.elapsed()/1000.0;
    parsedData[0] << time;
    parsedData[1] << qSin(time);
    parsedData[2] << qCos(time);
    parsedData[3] << qSin(2 * time);
    parsedData[4] << qCos(2 * time);
    // EMIT THE SIGNAL FOR NEW DATA
    emit(newDataAvailable(parsedData));
    ui->plot->replot();

    // EMPTY THE DATA BUFFER
    for(int k = 0; k<numData; k++) {
        parsedData[k].clear();
    }
#endif
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
