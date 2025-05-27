#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog){
    ui->setupUi(this);
    loadSettings();
}

SettingsDialog::~SettingsDialog(){
    delete ui;
}

void SettingsDialog::loadSettings() {
    QSettings settings("Simula", "SimVul");

    ui->hostEdit->setText(settings.value("host", "localhost").toString());
    ui->portEdit->setText(settings.value("port", "8529").toString());
    ui->plantUmlPathEdit->setText(settings.value("plantUmlPath", "").toString());
    ui->checkFullTextSearch->setChecked(settings.value("fullTextSearchEnabled", false).toBool());
}

void SettingsDialog::saveSettings() {
    QSettings settings("Simula", "SimVul");

    settings.setValue("host", ui->hostEdit->text());
    settings.setValue("port", ui->portEdit->text());
    settings.setValue("plantUmlPath", ui->plantUmlPathEdit->text());
    settings.setValue("fullTextSearchEnabled", ui->checkFullTextSearch->isChecked());
}

void SettingsDialog::accept() {
    saveSettings();
    QDialog::accept();  // call base class to close dialog
}
