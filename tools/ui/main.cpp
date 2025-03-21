#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "mainwindow.h"
#include "cvelistmodel.h"

int main(int argc, char *argv[]){
    QApplication app(argc, argv);

    qmlRegisterType<CVEListModel>("CVE", 1, 0, "CVEListModel");

    MainWindow window;
    window.show();

    return app.exec();
}
