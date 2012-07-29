#ifndef SNAKEWIZARDPANEL_H
#define SNAKEWIZARDPANEL_H

#include <QWidget>

namespace Ui {
    class SnakeWizardPanel;
}

class SnakeWizardPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SnakeWizardPanel(QWidget *parent = 0);
    ~SnakeWizardPanel();

private slots:
  void on_pushButton_4_clicked();

private:
    Ui::SnakeWizardPanel *ui;
};

#endif // SNAKEWIZARDPANEL_H
