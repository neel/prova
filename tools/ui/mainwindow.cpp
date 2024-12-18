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
#include <QJsonModel.hpp>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    _exuModel = new ExUModel{this};
    ui->exuTreeView->setModel(_exuModel);
    // ui->exuTreeView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    populate();
    std::cout << "Populated" << std::endl;
    connect(ui->exuTreeView, &QTreeView::doubleClicked, this, &MainWindow::exuSelected);
    connect(ui->exuTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, _exuModel ,[this](const QItemSelection& selected, const QItemSelection&)->void{
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
        nlohmann::json actions_properties = nlohmann::json::array();
        for(const auto& action: session->_actions){
            nlohmann::json properties = action->properties();
            properties["time"] = std::format("{:%T %F}", action->time());
            properties["open"] = action->operation();
            actions_properties.push_back(properties);
        }
        nlohmann::json artifact_properties = session->artifact()->properties();
        artifact_properties.erase("_key");
        nlohmann::json session_json = {
            {"artifact", artifact_properties},
            {"actions", actions_properties}
        };

        std::string json_str = session_json.dump();
        std::cout << json_str << std::endl;

        QJsonModel* json_model = new QJsonModel;
        QTreeView*  json_view  = new QTreeView;
        json_view->setAlternatingRowColors(true);
        json_view->setModel(json_model);
        json_model->loadJson(json_str.c_str());

        QMdiSubWindow* subWindow = ui->mdiArea->addSubWindow(json_view);
        subWindow->setWindowTitle(QString::fromStdString(std::format("Properties")));
        subWindow->show();
    }
}
