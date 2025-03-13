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
#include <fstream>
#include <QContextMenuEvent>

#include "exuvulnerabilitiesviewer.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    ui->setupUi(this);
    _exuModel = new ExUModel{this};
    ui->exuTreeView->setModel(_exuModel);
    ui->exuTreeView->viewport()->installEventFilter(this);
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

    connect(this, &MainWindow::vulnerabilitiesClicked, this, &MainWindow::showVulnerabilities);
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

    std::size_t i = 0;
    for(const auto& unit: exu_units){
        nlohmann::json dataset = nlohmann::json::array();
        unit->root()->flatten(dataset);
        std::string json_str = dataset.dump(4);
        std::string out_path = std::format("{}.syscalls.json", i++);
        std::ofstream outfile(out_path.c_str());
        if (outfile.is_open()) {
            outfile << json_str;
            outfile.close();
            std::cout << "Data successfully written to " << out_path << std::endl;
        } else {
            std::cerr << "Unable to open file for writing." << std::endl;
        }
    }

    std::cout << "ready " << std::endl;
    qDebug() << "Ready!";
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
            properties["operation"] = action->operation();
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

bool MainWindow::eventFilter(QObject* target, QEvent *event){
    if (target == ui->exuTreeView->viewport()) {
        QContextMenuEvent* e = dynamic_cast<QContextMenuEvent*>(event);
        if (event->type() == QEvent::ContextMenu && e!=0) {
            auto epos  = e->pos();
            auto gpos  = ui->exuTreeView->viewport()->mapToGlobal(epos);
            auto index = ui->exuTreeView->indexAt(epos);

            std::cout << index.row() << std::endl;

            QMenu context_menu;
            auto res_action = context_menu.addAction("Resources");
            auto vul_action = context_menu.addAction("Vulnerabilities");

            connect(vul_action, &QAction::triggered, [index, this](){
                emit vulnerabilitiesClicked(index.row());
            });

            context_menu.exec(gpos);

            return true;
        }
    }
    return false;
}

void MainWindow::showVulnerabilities(int row){
    std::vector<std::string> paths;

    const std::shared_ptr<prova::execution_unit>& unit = _exuModel->unit(row);
    for(const std::shared_ptr<prova::artifact>& artifact: *unit){
        const nlohmann::json& properties = artifact->properties();
        if(properties.count("path") > 0){
            std::string path = properties["path"].get<std::string>();
            paths.push_back(path);

            std::cout << path << std::endl;
        }
    }

    ExUVulnerabilitiesViewer* viewer = new ExUVulnerabilitiesViewer;
    for(const std::string& path: paths){
        viewer->request(QString::fromStdString(path));
    }
    viewer->show();
}
