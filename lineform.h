#ifndef LINEFORM_H
#define LINEFORM_H

#include <QWidget>
#include <QColorDialog>
#include "qcustomplot/qcustomplot.h"

struct linePropertiesT{
    unsigned int id;
    unsigned int xIndex;
    unsigned int yIndex;
    QPen pen;
};

namespace Ui {
class LineForm;
}

class LineForm : public QWidget
{
    Q_OBJECT

public:
    explicit LineForm(QWidget *parent = 0, QWidget *mainWidget = 0, int id = 0, QCustomPlot* plotParent = NULL, QCPAxisRect* axesRectParent = NULL);
    ~LineForm();
    const linePropertiesT & getLineProperties();

    void clearPlot();

private slots:
    void on_pushButton_color_clicked();
    void on_spinBox_xIndex_valueChanged(int arg1);
    void on_spinBox_yIndex_valueChanged(int arg1);
    void processNewData(QVector<QVector<double>>const &data);
    void on_comboBox_style_currentIndexChanged(int index);
    void on_comboBox_width_currentIndexChanged(int index);
    void on_checkBox_limitLen_toggled(bool checked);
    void on_comboBox_pointShape_currentIndexChanged(int index);

    void xWindowChanged(int);
    void yWindowChanged(int);

private:
    Ui::LineForm *ui;
    linePropertiesT lineProperties;
    void defaultLineProperties();
    QCustomPlot* plot;
    QCPAxisRect* axesRect;
    //QCPGraph* graph;
//    QCPGraph* curve;
    QCPCurve* curve;
    QCPGraph* point;
    QWidget* widget;
    QWidget* tab;
    double xWindow;
    double yWindow;

    QVector<QCPScatterStyle::ScatterShape> pointShapes;

signals:
};

#endif // LINEFORM_H
