#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "mainwindow.h"
#include "cvelistmodel.h"
#include "colorhelper.h"

int main(int argc, char *argv[]){
    QApplication app(argc, argv);

    qmlRegisterType<CVEListModel>("CVE", 1, 0, "CVEListModel");
    qmlRegisterSingletonType<ColorHelper>("Helpers", 1, 0, "ColorHelper", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)
        return new ColorHelper();
    });

    MainWindow window;
    window.show();

    return app.exec();
}
