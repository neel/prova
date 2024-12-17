#include "mainwindow.h"
#include "prova/store.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <QMessageBox>
#include <format>
#include "prova/execution_unit.h"
#include "prova/action.h"
#include <QSvgWidget>
#include <QTextEdit>
#include <QMdiSubWindow>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    _exuModel = new ExUModel{this};
    ui->exuTreeView->setModel(_exuModel);
    // ui->exuTreeView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    populate();
    std::cout << "Populated" << std::endl;
    connect(ui->exuTreeView, &QTreeView::doubleClicked, this, &MainWindow::exuSelected);
    connect(ui->exuTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, _exuModel ,[this](const QItemSelection &selected, const QItemSelection &deselected)->void{
        const auto selectedIndexes = selected.indexes();
        if(selectedIndexes.size() == 0){
            return;
        }
        const auto selectedIndex = selectedIndexes[0];
        if(selectedIndex.parent().isValid()){
            auto node = static_cast<ExUModel::tree_node*>(selectedIndex.internalPointer());
            _exuModel->updateHeaderLevel(node->level);
        } else {
            _exuModel->updateHeaderLevel(0);
        }
    });
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

    if(!index.parent().isValid()){
        const std::shared_ptr<prova::execution_unit>& unit = _exuModel->unit(index.row());
        std::filesystem::path image_path{std::format("{}.svg", index.row())};
        unit->render_svg(image_path);
        std::cout << "Rendered " << image_path<< std::endl;

        // Create a new SVG Widget
        QSvgWidget* svgWidget = new QSvgWidget(QString::fromStdString(image_path.string()));

        // Create a subwindow and set its widget to the SVG widget
        QMdiSubWindow* subWindow = ui->mdiArea->addSubWindow(svgWidget);
        subWindow->setWindowTitle(QString::fromStdString(std::format("ExU {}", index.row())));
        subWindow->show();
    } else {
        auto node = static_cast<ExUModel::tree_node*>(index.internalPointer());
        const prova::session* session = static_cast<const prova::session*>(node->data);
        nlohmann::json all_action_properties = nlohmann::json::array();
        for(const auto& action: session->_actions){
            nlohmann::json properties = action->properties();
            std::cout << properties << std::endl;
            all_action_properties.push_back(properties);
        }

        std::string json_str = all_action_properties.dump();

        QTextEdit* properties_window = new QTextEdit(this);
        properties_window->setText(QString::fromStdString(json_str));

        QMdiSubWindow* subWindow = ui->mdiArea->addSubWindow(properties_window);
        subWindow->setWindowTitle(QString::fromStdString(std::format("Properties")));
        subWindow->show();
    }
}
