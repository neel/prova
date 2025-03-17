#include "exuresourcechartviewer.h"
#include "ui_exuresourcechartviewer.h"
#include "sessionrectgroup.h"
#include "prova/execution_unit.h"
#include "prova/session.h"

ExUResourceChartViewer::ExUResourceChartViewer(std::shared_ptr<prova::execution_unit> unit, QWidget *parent): QWidget(parent), ui(new Ui::ExUResourceChartViewer){
    ui->setupUi(this);
    _scene = new QGraphicsScene{this};
    ui->graphicsView->setScene(_scene);

    SessionRectGroup* group = new SessionRectGroup{unit->root(), nullptr};
    _scene->addItem(group);

    // SessionGraphicsItem* rootItem = new SessionGraphicsItem(unit->root(), nullptr);
    // _scene->addItem(rootItem);
}

ExUResourceChartViewer::~ExUResourceChartViewer(){
    delete ui;
}
