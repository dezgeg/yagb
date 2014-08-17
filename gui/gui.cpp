#include "MainWindow.hpp"

#include <QApplication>
#include <unistd.h>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    bool trace = false;
    int opt;
    while ((opt = getopt(argc, argv, "t")) != -1) {
        switch (opt) {
            case 't':
                trace = true;
                break;
            default:
                fprintf(stderr, "usage: %s [-t] [rom]\n", argv[0]);
                return 1;
        }
    }
    const char* file = optind >= argc ? "test.bin" : argv[optind];

    MainWindow main(file, trace);
    main.show();

    return app.exec();
}
