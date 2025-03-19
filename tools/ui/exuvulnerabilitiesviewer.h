#ifndef EXUVULNERABILITIESVIEWER_H
#define EXUVULNERABILITIESVIEWER_H

#include <QWidget>

class QNetworkAccessManager;
class QQuickWidget;
class QNetworkReply;

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
    void replyReceived(const QString &keyword, QNetworkReply* reply);
    void replyEmpty(const QString &keyword);
    void cveReplyReceived(const QString &keyword, QNetworkReply* reply);
    void clearResults();
private:
    Ui::ExUVulnerabilitiesViewer *ui;
    QNetworkAccessManager*       _network;
    QQuickWidget*                _quickWidget;
    QSet<QString>                _cves;
};

#endif // EXUVULNERABILITIESVIEWER_H
