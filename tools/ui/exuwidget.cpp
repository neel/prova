#include "exuwidget.h"
#include <QSplitter>
#include <QTreeView>
#include "exuresourcechartviewer.h"
#include "exuvulnerabilitiesviewer.h"
#include "prova/action.h"
#include "prova/artifact.h"
#include <QJsonModel.hpp>

ExUWidget::ExUWidget(QWidget *parent): QWidget{parent} {
    _layout          = new QHBoxLayout{this};
    _vSplitter       = new QSplitter{Qt::Vertical, this};
    _hSplitter       = new QSplitter{Qt::Horizontal, _vSplitter};

    _chart           = new ExUResourceChartViewer{this};
    _vulnerabilities = new ExUVulnerabilitiesViewer{this};
    _sessionPropertyViewer = new QTreeView{this};

    _sessionPropertyModel = new QJsonModel{this};
    _sessionPropertyViewer->setModel(_sessionPropertyModel);
    _sessionPropertyViewer->setAlternatingRowColors(true);

    _layout->addWidget(_vSplitter);
    _vSplitter->addWidget(_chart);
    _hSplitter->addWidget(_vulnerabilities);
    _hSplitter->addWidget(_sessionPropertyViewer);

    connect(_chart, &ExUResourceChartViewer::exuSessionSelected, this, &ExUWidget::exuSessionSelected);
    connect(this, &ExUWidget::exuSessionSelected, this, &ExUWidget::exuSessionSelectedSlot);
}

ExUWidget::ExUWidget(std::shared_ptr<prova::execution_unit> unit, QWidget *parent): ExUWidget(parent){
    setUnit(unit);
}

void ExUWidget::setUnit(std::shared_ptr<prova::execution_unit> unit){
    _unit = unit;
    _chart->setUnit(_unit);
}

std::shared_ptr<prova::execution_unit> ExUWidget::unit(){
    return _unit;
}

void ExUWidget::exuSessionSelectedSlot(prova::session::ptr session, bool selected){
    if(!selected) {
        _sessionPropertyModel->loadJson("{}");
        return;
    }

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
    _sessionPropertyModel->loadJson(json_str.c_str());
    _sessionPropertyViewer->expandAll();

    _vulnerabilities->clearResults();
    if(artifact_properties.count("path") > 0){
        std::string path = artifact_properties["path"].get<std::string>();
        _vulnerabilities->request(QString::fromStdString(path));
    }
}


