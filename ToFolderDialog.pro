#-------------------------------------------------
#
# Project created by QtCreator 2016-08-26T13:42:56
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ToFolderDialog
TEMPLATE = app


SOURCES += main.cpp\
        tofolderdialog.cpp \
    folderdialog.cpp \
    gst/gst.c

HEADERS  += tofolderdialog.h \
    folderdialog.h \
    gst/gst.h

FORMS    += tofolderdialog.ui \
    folderdialog.ui

DISTFILES +=

win32 {
    RC_ICONS = icons/tofolder.ico
}
