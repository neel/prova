#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "prova/store.h"
#include "exumodel.h"

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
    void showSequenceDiagram(int row);
    void showResources(int row);
    void showSettingsDialog();
    void filterProcess(const QString& processName);
  private:
    Ui::MainWindow*         ui;
    ExUModel*               _exuModel;
    ExUFilterProxyModel*    _exuFilterProxyModel;
    QNetworkAccessManager*  _network;
    QNetworkDiskCache*      _cache;
  private:
    // std::vector<std::shared_ptr<prova::execution_unit>> _units;
    prova::store _store;
  private:
    QString _plantumlJarPath;
    void unpackPlantUmlJar();
};

#endif // MAINWINDOW_H
