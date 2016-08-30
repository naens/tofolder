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
    gst.cpp

HEADERS  += tofolderdialog.h \
    folderdialog.h \
    gst.h

FORMS    += tofolderdialog.ui \
    folderdialog.ui

DISTFILES += \
    TODO \
    LICENSE \
    README.md \
    icons/tofolder.ico

win32 {
    RC_ICONS = icons/tofolder.ico
}
