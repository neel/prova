#include "mainwindow.h"
#include "prova/store.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <QMessageBox>
#include <format>
#include "prova/execution_unit.h"
#include <QSvgWidget>
#include <QMdiSubWindow>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    _exuModel = new ExUModel{this};
    ui->exuTableView->setModel(_exuModel);
    ui->exuTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    populate();
    std::cout << "Populated" << std::endl;
    connect(ui->exuTableView, &QTableView::doubleClicked, this, &MainWindow::exuSelected);
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::populate(){
    _store.fetch();
    std::vector<std::shared_ptr<prova::execution_unit>> exu_units;
    _store.extract(exu_units);
    for(const auto& unit: exu_units){
        _exuModel->add_unit(unit);
    }
    std::cout << "Fetched " << exu_units.size() << std::endl;
}

void MainWindow::exuSelected(const QModelIndex& index){
    // QMessageBox::information(this, "Row Selected", QString("Row %1, Column %2 clicked").arg(index.row()).arg(index.column()));

    const std::shared_ptr<prova::execution_unit>& unit = _exuModel->unit(index.row());
    std::filesystem::path image_path{std::format("{}.svg", index.row())};
    unit->render_svg(image_path);
    std::cout << "Rendered " << image_path<< std::endl;

    // Create a new SVG Widget
    QSvgWidget* svgWidget = new QSvgWidget(QString::fromStdString(image_path.string()));

    // Create a subwindow and set its widget to the SVG widget
    QMdiSubWindow* subWindow = ui->mdiArea->addSubWindow(svgWidget);
    subWindow->setWindowTitle(QString::fromStdString(image_path.filename().string()));
    subWindow->show();
}
