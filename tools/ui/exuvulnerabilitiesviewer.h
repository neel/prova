#ifndef EXUVULNERABILITIESVIEWER_H
#define EXUVULNERABILITIESVIEWER_H

#include <QWidget>
#include "cvelistmodel.h"
#include <prova/execution_unit.h>

class QNetworkAccessManager;
class QQuickWidget;
class QNetworkReply;
class KeywordListModel;
class ExUVulnerabilitiesProgressWidget;
class QVBoxLayout;
class ExUCVESearchProgressScrollArea;
class CVEProxyModel;

namespace Ui {
class ExUVulnerabilitiesViewer;
}



class ExUVulnerabilitiesViewer : public QWidget{
    Q_OBJECT

public:
    explicit ExUVulnerabilitiesViewer(QNetworkAccessManager* network, CVEListModel::SearchPolicy searchPolicy, QWidget *parent = nullptr);
    ~ExUVulnerabilitiesViewer();

public:
    void setUnit(std::shared_ptr<prova::execution_unit> unit);
    void filter(const QString& keyword);
    void setNVDApiKey(const QString& apiKey);
private:
    Ui::ExUVulnerabilitiesViewer *ui;
    QNetworkAccessManager*       _network;
    CVEListModel*                _cveModel;
    KeywordListModel*            _keywordsModel;
    CVEProxyModel*               _cveFilterModel;
    QQuickWidget*                _quickWidget;
    QSet<QString>                _cves;
    ExUVulnerabilitiesProgressWidget* _progressArea;
    std::shared_ptr<prova::execution_unit> _unit;
    QMap<QString, QString>       _paths;
private slots:
    void responseReceivedSlot(const QString& keyword);
    void delayNextSearch();
    void searchNext();
    void setFilterText(const QString &text);
    void updateLabelCount();
};

#endif // EXUVULNERABILITIESVIEWER_H
