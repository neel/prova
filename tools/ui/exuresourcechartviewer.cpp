#include "exuresourcechartviewer.h"
#include "ui_exuresourcechartviewer.h"
#include "sessionrectgroup.h"
#include "prova/execution_unit.h"
#include "prova/session.h"
#include "exuscene.h"

ExUResourceChartViewer::ExUResourceChartViewer(QWidget *parent): QWidget(parent), ui(new Ui::ExUResourceChartViewer){
    ui->setupUi(this);
    _scene = new ExUScene{this};
    ui->graphicsView->setInteractive(true);
    ui->graphicsView->setScene(_scene);

    connect(_scene, &ExUScene::exuSessionSelected, this, &ExUResourceChartViewer::exuSessionSelected);
}

ExUResourceChartViewer::ExUResourceChartViewer(std::shared_ptr<prova::execution_unit> unit, QWidget *parent): ExUResourceChartViewer(parent){
    setUnit(unit);
}

ExUResourceChartViewer::~ExUResourceChartViewer(){
    delete ui;
}

void ExUResourceChartViewer::setUnit(std::shared_ptr<prova::execution_unit> unit){
    _unit = unit;
    SessionRectGroup* group = new SessionRectGroup{unit->root(), nullptr};
    _scene->addItem(group);
}
