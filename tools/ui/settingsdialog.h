#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();
    QVariant cvePolicyToVariant() const;
private:
    Ui::SettingsDialog *ui;
private slots:
    void loadSettings();
    void saveSettings();
    void accept() override;
};

#endif // SETTINGSDIALOG_H
