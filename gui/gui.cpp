#include "MainWindow.hpp"

#include <QApplication>
#include <unistd.h>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    bool gbc = false;
    bool trace = false;
    int opt;
    while ((opt = getopt(argc, argv, "ct")) != -1) {
        switch (opt) {
            case 'c':
                gbc = true;
                break;
            case 't':
                trace = true;
                break;
            default:
                fprintf(stderr, "usage: %s [-t] [-c] [rom]\n", argv[0]);
                return 1;
        }
    }
    const char* file = optind >= argc ? "test.bin" : argv[optind];

    try {
        MainWindow main(file, gbc, trace);
        main.show();

        return app.exec();
    } catch (const char* msg) {
        fprintf(stderr, "error: %s\n", msg);
        return 1;
    }
}
