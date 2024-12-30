#include "songlist.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    SongList w;
    w.show();

    return a.exec();
}
