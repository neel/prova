#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "mainwindow.h"
#include "cvelistmodel.h"
#include "keywordlistmodel.h"
#include "colorhelper.h"
#include "cveproxymodel.h"
#include <QStyleFactory>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QDir>

#include <iostream>

// #include "directorytrie.h"

int main(int argc, char *argv[]){
    // DirectoryTrie  dirTrie;
    // dirTrie.insert({"a", "b", "c", "d"});
    // dirTrie.insert({"a", "b", "x", "y"});
    // dirTrie.insert({"k", "b", "x", "p"});
    // // dirTrie.insert({"k", "t", "p", "z"});
    // qDebug() << dirTrie.suffixesMap(0);

    QStringList app_styles = QStyleFactory::keys();
    std::cout << "Available Qt styles on this platform:";
    for(const QString& app_style: app_styles) {
        std::cout << app_style.toStdString() << " ";
    }
    std::cout << std::endl;

    QApplication app(argc, argv);

    QQmlEngine engine;
    qDebug() << engine.importPathList();
    for (const auto &path : engine.importPathList()) {
        QDir dir(path + "/QtQuick/Controls");
        if (dir.exists()) {
            std::cout << "Styles in" << dir.absolutePath().toStdString() << ":";
            auto list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for(const auto& item: list){
                std::cout << item.toStdString() << " ";
            }
            std::cout << std::endl;
        }
    }

    // QQuickStyle::setStyle("Basic");

    // qmlRegisterType<CVEListModel>("CVE", 1, 0, "CVEListModel");
    qmlRegisterType<CVEProxyModel>("CVE", 1, 0, "CVEListModel");
    qmlRegisterType<KeywordListModel>("Keywords", 1, 0, "KeywordListModel");
    qmlRegisterSingletonType<ColorHelper>("Helpers", 1, 0, "ColorHelper", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)
        return new ColorHelper();
    });

    MainWindow window;
    window.show();

    return app.exec();
}
