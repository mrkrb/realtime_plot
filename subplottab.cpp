#include "subplottab.h"
#include "ui_subplottab.h"

SubplotTab::SubplotTab(QWidget *parent, int id, QCustomPlot* plotParent) :
    QWidget(parent),
    ui(new Ui::SubplotTab)
{
    ui->setupUi(this);
    mainWidget = parent;
    _id = id;
    plot = plotParent;

    axesRect = new QCPAxisRect(plot);
    plot->plotLayout()->addElement(id-1, 0, axesRect);
    plot->replot();

    ui->checkBox_scrollX->setChecked(true);
    ui->checkBox_resizeY->setChecked(true);

    connect(plot, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));
    connect(plot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress()));
    connect(plot, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel()));

    addLine();

    enableDisableXWindow(ui->checkBox_scrollX->isChecked());
    enableDisableYWindow(ui->checkBox_scrollY->isChecked());

    ui->horizontalSlider_xWindow->setValue(500);
    ui->horizontalSlider_yWindow->setValue(500);
}

SubplotTab::~SubplotTab()
{
    delete ui;
}

int SubplotTab::id() {
    return _id;
}

void SubplotTab::addLine() {
    int nLines = countLines();
    LineForm* newLine = new LineForm(this, mainWidget, nLines+1, plot, axesRect);
    connect(ui->horizontalSlider_xWindow, SIGNAL(valueChanged(int)), newLine, SLOT(xWindowChanged(int)));
    connect(ui->horizontalSlider_yWindow, SIGNAL(valueChanged(int)), newLine, SLOT(yWindowChanged(int)));
    newLine->setObjectName("LineForm_" + QString::number(nLines + 1));
    newLine->findChildren<QGroupBox*>().first()->setTitle("Line " + QString::number(nLines + 1));
    ui->verticalLayout_2->insertWidget(nLines, newLine);
}

void SubplotTab::removeLine() {
    QWidget* toRemove = this->findChildren<LineForm*>().last();
    ui->verticalLayout_2->removeWidget(toRemove);
    delete toRemove;
}

void SubplotTab::on_spinBox_numberLines_valueChanged(int arg1)
{
    int nlines = countLines();
    if(nlines < arg1) {
        for(int i = nlines; i < arg1; i++) {
            addLine();
        }
    } else {
        for(int i = nlines; i > arg1; i--) {
            removeLine();
        }
    }
}

int SubplotTab::countLines() {
    return ui->verticalLayout_2->count();
}

void SubplotTab::on_checkBox_scrollX_toggled(bool checked)
{
    ui->checkBox_resizeX->setEnabled(!checked);
    enableDisableXWindow(checked);

}

void SubplotTab::on_checkBox_resizeX_toggled(bool checked)
{
    ui->checkBox_scrollX->setEnabled(!checked);
}

void SubplotTab::on_checkBox_scrollY_toggled(bool checked)
{
    ui->checkBox_resizeY->setEnabled(!checked);
    enableDisableYWindow(checked);
}

void SubplotTab::on_checkBox_resizeY_toggled(bool checked)
{
    ui->checkBox_scrollY->setEnabled(!checked);
}

void SubplotTab::selectionChanged() {
    if (axesRect->axis(QCPAxis::atBottom, 0)->selectedParts().testFlag(QCPAxis::spAxis) || axesRect->axis(QCPAxis::atBottom, 0)->selectedParts().testFlag(QCPAxis::spTickLabels)) {
        axesRect->axis(QCPAxis::atBottom, 0)->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    }
    if (axesRect->axis(QCPAxis::atLeft, 0)->selectedParts().testFlag(QCPAxis::spAxis) || axesRect->axis(QCPAxis::atLeft, 0)->selectedParts().testFlag(QCPAxis::spTickLabels)) {
        axesRect->axis(QCPAxis::atLeft, 0)->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    }
}

void SubplotTab::mousePress() {
    if (axesRect->axis(QCPAxis::atBottom, 0)->selectedParts().testFlag(QCPAxis::spAxis) && !axesRect->axis(QCPAxis::atLeft, 0)->selectedParts().testFlag(QCPAxis::spAxis)) {
        axesRect->setRangeDrag(Qt::Horizontal);
    } else if (axesRect->axis(QCPAxis::atLeft, 0)->selectedParts().testFlag(QCPAxis::spAxis) && !axesRect->axis(QCPAxis::atBottom, 0)->selectedParts().testFlag(QCPAxis::spAxis)) {
        axesRect->setRangeDrag(Qt::Vertical);
    } else {
        axesRect->setRangeDrag(Qt::Horizontal|Qt::Vertical);
    }
}

void SubplotTab::mouseWheel() {
    if (axesRect->axis(QCPAxis::atBottom, 0)->selectedParts().testFlag(QCPAxis::spAxis) && !axesRect->axis(QCPAxis::atLeft, 0)->selectedParts().testFlag(QCPAxis::spAxis)) {
        axesRect->setRangeZoom(Qt::Horizontal);
    } else if (axesRect->axis(QCPAxis::atLeft, 0)->selectedParts().testFlag(QCPAxis::spAxis) && !axesRect->axis(QCPAxis::atBottom, 0)->selectedParts().testFlag(QCPAxis::spAxis)) {
        axesRect->setRangeZoom(Qt::Vertical);
    } else {
        axesRect->setRangeZoom(Qt::Horizontal|Qt::Vertical);
    }
}

void SubplotTab::enableDisableXWindow(bool state){
    for(int i = 0; i<ui->horizontalLayout_xWindow->count(); i++) {
        QWidget* w = ui->horizontalLayout_xWindow->itemAt(i)->widget();
        if(w) {
            w->setEnabled(state);
        }
    }
}

void SubplotTab::enableDisableYWindow(bool state){
    for(int i = 0; i<ui->horizontalLayout_yWindow->count(); i++) {
        QWidget* w = ui->horizontalLayout_yWindow->itemAt(i)->widget();
        if(w) {
            w->setEnabled(state);
        }
    }
}

void SubplotTab::on_horizontalSlider_xWindow_valueChanged(int value)
{
    ui->label_xWindow->setText(QString::number(value/100.0, 'f', 2) + "s");
}

void SubplotTab::on_horizontalSlider_yWindow_valueChanged(int value)
{
    ui->label_yWindow->setText(QString::number(value/100.0, 'f', 2) + "s");
}
