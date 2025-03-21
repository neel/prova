#ifndef EXUVULNERABILITIESVIEWER_H
#define EXUVULNERABILITIESVIEWER_H

#include <QWidget>

class QNetworkAccessManager;
class QQuickWidget;
class QNetworkReply;
class CVEListModel;

namespace Ui {
class ExUVulnerabilitiesViewer;
}

class ExUVulnerabilitiesViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ExUVulnerabilitiesViewer(QWidget *parent = nullptr);
    ~ExUVulnerabilitiesViewer();

public:
    void request(const QString& keyword);
    void filter(const QString& keyword);
private:
    Ui::ExUVulnerabilitiesViewer *ui;
    QNetworkAccessManager*       _network;
    CVEListModel*                _cveModel;
    QQuickWidget*                _quickWidget;
    QSet<QString>                _cves;
};

#endif // EXUVULNERABILITIESVIEWER_H
