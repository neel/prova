#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "prova/store.h"
#include "exumodel.h"
#include <QQmlApplicationEngine>

namespace Ui {
class MainWindow;
}

class QNetworkAccessManager;
class QNetworkDiskCache;

namespace prova{
struct execution_unit;
struct store;
}

class ExUFilterProxyModel;

QT_BEGIN_NAMESPACE
class QActionGroup;
class QQuickWidget;
QT_END_NAMESPACE

class MainWindow : public QMainWindow{
    Q_OBJECT
  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
  public slots:
    void populate();
  private slots:
    void exuSelected(const QModelIndex& index);
  private:
    bool eventFilter(QObject* target, QEvent* event);
  signals:
    void sequenceClicked(int row);
    void resourcesClicked(int row);
  private slots:
    void applyWidgetStyle(QAction* act);
    void applyQuickStyle(QAction* act);
    void showSequenceDiagram(int row);
    void showResources(int row);
    void showSettingsDialog();
    void filterProcess(const QString& processName);
  private:
    void buildStyleMenus();                 // helper called from ctor
    void reloadQmlRoot();                   // clears + re-loads the QML tree
  private:
    Ui::MainWindow*         ui;
    ExUModel*               _exuModel;
    ExUFilterProxyModel*    _exuFilterProxyModel;
    QNetworkAccessManager*  _network;
    QNetworkDiskCache*      _cache;
    QPointer<QQmlEngine>  m_engine;         // already created elsewhere in your class
    QPointer<QQuickWidget> m_quickWidget;   // or QQuickView*, etc.

    QActionGroup *m_widgetStyleGroup {nullptr};
    QActionGroup *m_quickStyleGroup  {nullptr};
  private:
    // std::vector<std::shared_ptr<prova::execution_unit>> _units;
    prova::store _store;
  private:
    QString _plantumlJarPath;
    void unpackPlantUmlJar();
};

#endif // MAINWINDOW_H
