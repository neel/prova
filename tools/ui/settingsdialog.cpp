#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>
#include "cvelistmodel.h"

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog){
    ui->setupUi(this);
    loadSettings();

    ui->cveSearchCombo->clear();
    ui->cveSearchCombo->addItem("NIST NVD", static_cast<int>(CVEListModel::SearchPolicy::nvd_nist_json));
    ui->cveSearchCombo->addItem("Mitre CVE", static_cast<int>(CVEListModel::SearchPolicy::mitre_html));
}

SettingsDialog::~SettingsDialog(){
    delete ui;
}

QVariant SettingsDialog::cvePolicyToVariant() const {
    return ui->cveSearchCombo->currentData();
}

void SettingsDialog::loadSettings() {
    QSettings settings("Simula", "SimVul");

    ui->hostEdit->setText(settings.value("host", "localhost").toString());
    ui->portEdit->setText(settings.value("port", "8529").toString());
    ui->userEdit->setText(settings.value("user", "root").toString());
    ui->passEdit->setText(settings.value("pass", "").toString());
    ui->dbEdit  ->setText(settings.value("db",   "").toString());

    ui->apiKeyNVDLineEdit->setText(settings.value("apiKeyNVD", "").toString());

    ui->plantUmlPathEdit->setText(settings.value("plantUmlPath", "").toString());

    QVariant v = settings.value("cvePolicy", static_cast<int>(CVEListModel::SearchPolicy::nvd_nist_json));
    int idx = ui->cveSearchCombo->findData(v);
    if (idx < 0) idx = 0;
    ui->cveSearchCombo->setCurrentIndex(idx);
}

void SettingsDialog::saveSettings() {
    QSettings settings("Simula", "SimVul");

    settings.setValue("host", ui->hostEdit->text());
    settings.setValue("port", ui->portEdit->text());
    settings.setValue("user", ui->userEdit->text());
    settings.setValue("pass", ui->passEdit->text());
    settings.setValue("db",   ui->dbEdit->text());

    settings.setValue("plantUmlPath", ui->plantUmlPathEdit->text());
    settings.setValue("cvePolicy", cvePolicyToVariant());
    settings.setValue("apiKeyNVD", ui->apiKeyNVDLineEdit->text());
}

void SettingsDialog::accept() {
    saveSettings();
    QDialog::accept();  // call base class to close dialog
}
