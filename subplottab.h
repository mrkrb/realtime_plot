#ifndef SUBPLOTTAB_H
#define SUBPLOTTAB_H

#include <QWidget>
#include <QDebug>
#include "lineform.h"
#include <QGroupBox>
#include "qcustomplot/qcustomplot.h"

namespace Ui {
class SubplotTab;
}

class SubplotTab : public QWidget
{
    Q_OBJECT

public:
    explicit SubplotTab(QWidget *parent = 0, int id = 0, QCustomPlot* plotParent = NULL);
    ~SubplotTab();
    int id();

    void clearPlot();

public slots:
    void link_x_windows_check_changed(bool b);

private slots:
    void on_spinBox_numberLines_valueChanged(int arg1);
    void on_checkBox_scrollX_toggled(bool checked);
    void on_checkBox_resizeX_toggled(bool checked);
    void on_checkBox_scrollY_toggled(bool checked);
    void on_checkBox_resizeY_toggled(bool checked);
    void on_horizontalSlider_xWindow_valueChanged(int value);

    void selectionChanged();
    void mousePress();
    void mouseWheel();

    void on_horizontalSlider_yWindow_valueChanged(int value);


    void link_x_windows_value_changed(int  v);

private:
    QWidget* mainWidget;

    void addLine();
    void removeLine();
    int countLines();
    void enableDisableXWindow(bool state);
    void enableDisableYWindow(bool state);

    Ui::SubplotTab *ui;
    int _id;
    QCustomPlot* plot;
    QCPAxisRect* axesRect;


};

#endif // SUBPLOTTAB_H
