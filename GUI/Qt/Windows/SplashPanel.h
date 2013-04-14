#ifndef SPLASHPANEL_H
#define SPLASHPANEL_H

#include <QWidget>

namespace Ui {
class SplashPanel;
}

class SplashPanel : public QWidget
{
  Q_OBJECT
  
public:
  explicit SplashPanel(QWidget *parent = 0);
  ~SplashPanel();

private:
  Ui::SplashPanel *ui;
};

#endif // SPLASHPANEL_H
