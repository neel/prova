#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "prova/store.h"
#include "exumodel.h"

namespace Ui {
class MainWindow;
}

namespace prova{
struct execution_unit;
struct store;
}

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
    void resourcesClicked(int row);
    void vulnerabilitiesClicked(int row);
  private slots:
    void showResources(int row);
    void showVulnerabilities(int row);
    void showSettingsDialog();
  private:
    Ui::MainWindow *ui;
    ExUModel* _exuModel;
  private:
    // std::vector<std::shared_ptr<prova::execution_unit>> _units;
    prova::store _store;
};

#endif // MAINWINDOW_H
