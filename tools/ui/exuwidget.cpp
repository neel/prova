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

    _layout->setSpacing(0);
    _layout->setContentsMargins(0, 0, 0, 0);

    _resourceLifetimeViewer = new ExUResourceChartViewer{this};
    _vulnerabilitiesViewer  = new ExUVulnerabilitiesViewer{this};
    _sessionPropertyViewer  = new QTreeView{this};

    _sessionPropertyModel = new QJsonModel{this};
    _sessionPropertyViewer->setModel(_sessionPropertyModel);
    _sessionPropertyViewer->setAlternatingRowColors(true);

    _layout->addWidget(_vSplitter);
    _vSplitter->addWidget(_resourceLifetimeViewer);
    _hSplitter->addWidget(_vulnerabilitiesViewer);
    _hSplitter->addWidget(_sessionPropertyViewer);

    connect(_resourceLifetimeViewer, &ExUResourceChartViewer::exuSessionSelected, this, &ExUWidget::exuSessionSelected);
    connect(this, &ExUWidget::exuSessionSelected, this, &ExUWidget::exuSessionSelectedSlot);
}

ExUWidget::ExUWidget(std::shared_ptr<prova::execution_unit> unit, QWidget *parent): ExUWidget(parent){
    setUnit(unit);
}

void ExUWidget::setUnit(std::shared_ptr<prova::execution_unit> unit){
    _unit = unit;
    _resourceLifetimeViewer->setUnit(_unit);

    for(auto artifact: *_unit){
        nlohmann::json artifact_properties = artifact->properties();
        if(artifact_properties.count("path") > 0){
            std::string path = artifact_properties["path"].get<std::string>();
            _vulnerabilitiesViewer->request(QString::fromStdString(path));
        }
    }
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

    // if(artifact_properties.count("path") > 0){
    //     std::string path = artifact_properties["path"].get<std::string>();
    //     _vulnerabilitiesViewer->request(QString::fromStdString(path));
    //     _vulnerabilitiesViewer->filter(QString::fromStdString(path));
    // }
}


