#include "jsoneditor.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    JsonEditor w;
    w.show();
    return a.exec();
}
