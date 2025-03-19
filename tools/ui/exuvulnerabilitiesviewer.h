#ifndef EXUVULNERABILITIESVIEWER_H
#define EXUVULNERABILITIESVIEWER_H

#include <QWidget>

class QNetworkAccessManager;
class QQuickWidget;

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
signals:
    void jsonReady(QVariant);
private slots:
    void updateJsonData(const QVariant& data);
public slots:
    void replyReceived();
    void cveReplyReceived();
    void clearResults();
private:
    Ui::ExUVulnerabilitiesViewer *ui;
    QNetworkAccessManager*       _network;
    QQuickWidget*                _quickWidget;
    QSet<QString>                _cves;
};

#endif // EXUVULNERABILITIESVIEWER_H
