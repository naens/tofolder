#include "tofolderdialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ToFolderDialog w;
    w.show();

    return a.exec();
}
