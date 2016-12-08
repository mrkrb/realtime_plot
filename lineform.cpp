#include "lineform.h"
#include "ui_lineform.h"

LineForm::LineForm(QWidget *parent, QWidget *mainWidget, int id, QCustomPlot* plotParent, QCPAxisRect *axesRectParent) :
    QWidget(parent),
    ui(new Ui::LineForm)
{
    ui->setupUi(this);

    lineProperties.id = id;
    defaultLineProperties();

    tab = parent;

    plot = plotParent;
    axesRect = axesRectParent;
    widget = mainWidget;
    // GRAPH
//    graph = new QCPGraph(axesRect->axis(QCPAxis::atBottom), axesRect->axis(QCPAxis::atLeft));
//    graph->setPen(lineProperties.pen);
//    plot->addPlottable(graph);

    // CURVE
    curve = new QCPCurve(axesRect->axis(QCPAxis::atBottom), axesRect->axis(QCPAxis::atLeft));
    curve->setPen(lineProperties.pen);
    plot->addPlottable(curve);

    // POINT
    point = new QCPGraph(axesRect->axis(QCPAxis::atBottom), axesRect->axis(QCPAxis::atLeft));
    point->setPen(lineProperties.pen);
    point->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 5));
    plot->addPlottable(point);

    // STYLING THE COLOR BUTTON
    QString qss = QString("background-color: %1").arg(lineProperties.pen.color().name()) + ";border-style: outset;border-width: 2px;border-radius: 10px;border-color: white;";
    ui->pushButton_color->setStyleSheet(qss);

    connect(mainWidget, SIGNAL(newDataAvailable(QVector<QVector<double>>const &)), this, SLOT(processNewData(QVector<QVector<double>>const &)));

    // LINESTYLE SETUP
    int iconWidth  = 50;
    int iconHeight = 1;
    ui->comboBox_style->setIconSize(QSize(iconWidth, iconHeight));
    QList<QString> lineStyleNames;
    lineStyleNames << "none" << "solid" << "dash" << "dot" << "dash dot" << "dash dot dot";
    for (int lineStyle = Qt::NoPen; lineStyle < Qt::CustomDashLine; lineStyle++)
        {
            QPixmap pix(iconWidth, iconHeight);
            pix.fill(Qt::white);

            QBrush brush(Qt::black);
            QPen pen(brush, 1, (Qt::PenStyle)lineStyle);

            QPainter painter(&pix);
            painter.setPen(pen);
            painter.drawLine(0, iconHeight/2, iconWidth, iconHeight/2);

            ui->comboBox_style->addItem(QIcon(pix), lineStyleNames.at(lineStyle));
        }
    ui->comboBox_style->setCurrentIndex(1);
    // LINE WIDTH
    iconHeight = 6;
    ui->comboBox_width->setIconSize(QSize(iconWidth, iconHeight));
    for (int width = 1; width < 6; width++)
        {
            QPixmap pix(iconWidth, iconHeight);
            pix.fill(Qt::white);

            QBrush brush(Qt::black);
            QPen pen(brush, (double)width, Qt::SolidLine);


            QPainter painter(&pix);
            painter.setPen(pen);
            painter.drawLine(0, iconHeight/2, iconWidth, iconHeight/2);

            ui->comboBox_width->addItem(QIcon(pix),QString::number(width));
        }
    // LIMIT LAST
    ui->checkBox_limitLen->setChecked(false);
    // POINT STYLE
    pointShapes << QCPScatterStyle::ssCustom;
    pointShapes << QCPScatterStyle::ssCross;
    pointShapes << QCPScatterStyle::ssPlus;
    pointShapes << QCPScatterStyle::ssCircle;
    pointShapes << QCPScatterStyle::ssDisc;
    pointShapes << QCPScatterStyle::ssSquare;
    pointShapes << QCPScatterStyle::ssDiamond;
    pointShapes << QCPScatterStyle::ssStar;
    pointShapes << QCPScatterStyle::ssTriangle;
    pointShapes << QCPScatterStyle::ssTriangleInverted;
    pointShapes << QCPScatterStyle::ssCrossSquare;
    pointShapes << QCPScatterStyle::ssPlusSquare;
    pointShapes << QCPScatterStyle::ssCrossCircle;
    pointShapes << QCPScatterStyle::ssPlusCircle;
    pointShapes << QCPScatterStyle::ssPeace;
    for(int i=0; i<pointShapes.count(); i++) {
        QCPScatterStyle ss = pointShapes.at(i);
        ss.setSize(10);
        QPixmap pm(14, 14);
        QCPPainter qp(&pm);
        qp.fillRect(0, 0, 14, 14, QBrush(Qt::white, Qt::SolidPattern));
        ss.applyTo(&qp, QPen(Qt::black));
        ss.drawShape(&qp, 7, 7);
        QIcon   icon = QIcon(pm);
        ui->comboBox_pointShape->addItem(icon, "");
    }
    ui->comboBox_pointShape->setCurrentIndex(4);
}

LineForm::~LineForm()
{
    delete ui;
}

const linePropertiesT & LineForm::getLineProperties(){
    return lineProperties;
}

void LineForm::defaultLineProperties(){
    lineProperties.pen.setWidth(1);
    lineProperties.pen.setStyle(Qt::SolidLine);
    //// default color = blue
    //lineProperties.pen.setColor(Qt::blue);
    //// random first color
    //QStringList colorNames = QColor::colorNames();
    //qsrand(time(NULL));
    //lineProperties.color = colorNames.at(qrand() % (colorNames.length() + 1));
    //// list for standard first color cicling
    QList<QColor> basicColors({Qt::blue, Qt::red, Qt::green, Qt::black,  Qt::magenta});
    lineProperties.pen.setColor(basicColors.at((lineProperties.id - 1) % basicColors.length()));

    lineProperties.xIndex = 1;
    lineProperties.yIndex = 2;
}

void LineForm::on_pushButton_color_clicked()
{
    QColor color = QColorDialog::getColor(lineProperties.pen.color());
    if(color.isValid()) {
        lineProperties.pen.setColor(color);
        QString qss = QString("background-color: %1").arg(color.name()) + ";border-style: outset;border-width: 2px;border-radius: 10px;border-color: white;";
        ui->pushButton_color->setStyleSheet(qss);

//        graph->setPen(lineProperties.pen);
        curve->setPen(lineProperties.pen);
        point->setPen(lineProperties.pen);
    }
}

void LineForm::on_spinBox_xIndex_valueChanged(int arg1)
{
//    graph->clearData();
    curve->clearData();
    lineProperties.xIndex = arg1;
}

void LineForm::on_spinBox_yIndex_valueChanged(int arg1)
{
//    graph->clearData();
    curve->clearData();
    lineProperties.yIndex = arg1;
}

void LineForm::processNewData(QVector<QVector<double> > const &data) {
//    qDebug() << "updateing line " << lineProperties.id;

    unsigned int numData    = data.length();
    if(lineProperties.xIndex <= numData && lineProperties.yIndex <= numData) {
//        graph->addData(data[lineProperties.xIndex - 1], data[lineProperties.yIndex - 1]);
        //// CURVE
        curve->addData(data[0], data[lineProperties.xIndex - 1], data[lineProperties.yIndex - 1]);
        if(ui->checkBox_limitLen->isChecked()) {
            curve->removeDataBefore(data[0].last() - ui->doubleSpinBox_limitLen->value());
        }
        point->setData(data[lineProperties.xIndex - 1].mid(data[0].length()-1), data[lineProperties.yIndex - 1].mid(data[0].length()-1));
    } else {
        ui->spinBox_xIndex->setMaximum(numData);
        ui->spinBox_yIndex->setMaximum(numData);
    }

    //// ALIGNMENT
    QList<QCheckBox*> scrollSettings = tab->findChildren<QCheckBox*>();
    if( scrollSettings.at(0)->isChecked() ) {
        axesRect->axis(QCPAxis::atBottom)->setRange(data[lineProperties.xIndex - 1].last() + xWindow*0.1 - xWindow , data[lineProperties.xIndex - 1].last() + xWindow*0.1);
    } else if (scrollSettings.at(1)->isChecked()) {
        curve->rescaleKeyAxis();
    }

    if( scrollSettings.at(2)->isChecked() ) {
        axesRect->axis(QCPAxis::atLeft)->setRange(data[lineProperties.yIndex - 1].last() + yWindow*0.1, data[lineProperties.yIndex - 1].last() - yWindow + yWindow*0.1);
    } else if (scrollSettings.at(3)->isChecked()) {
        curve->rescaleValueAxis();
    }

}

void LineForm::on_comboBox_style_currentIndexChanged(int index)
{
    lineProperties.pen.setStyle(Qt::PenStyle(index));
    curve->setPen(lineProperties.pen);
}

void LineForm::on_comboBox_width_currentIndexChanged(int index)
{
    lineProperties.pen.setWidth(index+1);
    curve->setPen(lineProperties.pen);
}

void LineForm::on_checkBox_limitLen_toggled(bool checked)
{
    ui->doubleSpinBox_limitLen->setEnabled(checked);
    ui->label_style_limitLen->setEnabled(checked);

}

void LineForm::on_comboBox_pointShape_currentIndexChanged(int index)
{
    point->setScatterStyle(QCPScatterStyle(pointShapes.at(index), 5));
}

void LineForm::xWindowChanged(int value){
    xWindow = value/100.0;
}

void LineForm::yWindowChanged(int value){
    yWindow = value/100.0;
}
