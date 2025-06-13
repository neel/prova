#ifndef EXUWIDGET_H
#define EXUWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include "prova/execution_unit.h"
#include "prova/session.h"

class QSplitter;
class ExUResourceChartViewer;
class ExUVulnerabilitiesViewer;
class QTreeView;
class QJsonModel;
class QNetworkAccessManager;

class ExUWidget : public QWidget{
    Q_OBJECT
public:
    explicit ExUWidget(QNetworkAccessManager* network, QWidget *parent = nullptr);
    ExUWidget(std::shared_ptr<prova::execution_unit> unit, QNetworkAccessManager* network, QWidget *parent = nullptr);
    void setUnit(std::shared_ptr<prova::execution_unit> unit);
    std::shared_ptr<prova::execution_unit> unit();
    void setNVDApiKey(const QString& apiKey);
private:
    QHBoxLayout* _layout;
    QSplitter*   _vSplitter;
    QSplitter*   _hSplitter;
    ExUResourceChartViewer* _resourceLifetimeViewer;
    ExUVulnerabilitiesViewer* _vulnerabilitiesViewer;
    QTreeView*                _sessionPropertyViewer;
    QJsonModel*               _sessionPropertyModel;
private:
    QNetworkAccessManager* _network;
    std::shared_ptr<prova::execution_unit> _unit;
signals:
    void exuSessionSelected(prova::session::ptr, bool);
private slots:
    void exuSessionSelectedSlot(prova::session::ptr, bool);
};

#endif // EXUWIDGET_H
