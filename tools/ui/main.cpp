#include <QApplication>
#include "mainwindow.h"

#include "prova/store.h"

int main(int argc, char *argv[]){
    // prova::store store;
    // store.fetch();
    // // store.uml(std::cout);
    // store.extract_all();

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
