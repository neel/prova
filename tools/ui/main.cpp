#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "mainwindow.h"
#include "cvelistmodel.h"
#include "colorhelper.h"
#include "cveproxymodel.h"
#include <QStyleFactory>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QDir>

#include "directorytrie.h"

int main(int argc, char *argv[]){
    DirectoryTrie  dirTrie;
    dirTrie.insert({"a", "b", "c", "d"});
    dirTrie.insert({"a", "b", "x", "y"});
    dirTrie.insert({"k", "b", "x", "p"});
    // dirTrie.insert({"k", "t", "p", "z"});
    qDebug() << dirTrie.suffixes(0);

    QStringList app_styles = QStyleFactory::keys();
    qDebug() << "Available Qt styles on this platform:" << app_styles;

    QApplication app(argc, argv);

    QQmlEngine engine;
    for (const auto &path : engine.importPathList()) {
        QDir dir(path + "/QtQuick/Controls");
        if (dir.exists()) {
            qDebug() << "Styles in" << dir.absolutePath() << ":"
                     << dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        }
    }

    QQuickStyle::setStyle("Basic");

    // qmlRegisterType<CVEListModel>("CVE", 1, 0, "CVEListModel");
    qmlRegisterType<CVEProxyModel>("CVE", 1, 0, "CVEListModel");
    qmlRegisterSingletonType<ColorHelper>("Helpers", 1, 0, "ColorHelper", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)
        return new ColorHelper();
    });

    MainWindow window;
    window.show();

    return app.exec();
}
