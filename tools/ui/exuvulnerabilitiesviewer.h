#ifndef EXUVULNERABILITIESVIEWER_H
#define EXUVULNERABILITIESVIEWER_H

#include <QWidget>
#include <prova/execution_unit.h>

class QNetworkAccessManager;
class QQuickWidget;
class QNetworkReply;
class CVEListModel;
class ExUVulnerabilitiesProgressWidget;
class QVBoxLayout;
class ExUCVESearchProgressScrollArea;

namespace Ui {
class ExUVulnerabilitiesViewer;
}



class ExUVulnerabilitiesViewer : public QWidget{
    Q_OBJECT

public:
    explicit ExUVulnerabilitiesViewer(QNetworkAccessManager* network, QWidget *parent = nullptr);
    ~ExUVulnerabilitiesViewer();

public:
    void setUnit(std::shared_ptr<prova::execution_unit> unit);
    void filter(const QString& keyword);
private:
    Ui::ExUVulnerabilitiesViewer *ui;
    QNetworkAccessManager*       _network;
    CVEListModel*                _cveModel;
    QQuickWidget*                _quickWidget;
    QSet<QString>                _cves;
    ExUVulnerabilitiesProgressWidget* _progressArea;
    std::shared_ptr<prova::execution_unit> _unit;
    QSet<QString>               _paths;
private slots:
    void responseReceivedSlot(const QString& keyword);
};

#endif // EXUVULNERABILITIESVIEWER_H
