#-------------------------------------------------
#
# Project created by QtCreator 2016-04-28T14:04:48
#
#-------------------------------------------------

QT       += core gui network printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = socket_plot
TEMPLATE = app


SOURCES += main.cpp\
           widget.cpp\
           qcustomplot/qcustomplot.cpp \
           subplottab.cpp \
           lineform.cpp

HEADERS  += widget.h\
            qcustomplot/qcustomplot.h \
            subplottab.h \
            lineform.h

FORMS    += widget.ui \
            subplottab.ui \
            lineform.ui
