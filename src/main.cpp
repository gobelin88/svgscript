#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    if(a.arguments().size()>1)
    {
        w.slot_direct_load(a.arguments()[1]);
        w.slot_run();
    }

    w.show();

    return a.exec();
}
