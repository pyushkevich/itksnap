#ifndef OVERLAYROLEPAGE_H
#define OVERLAYROLEPAGE_H

#include <QWidget>
#include "ImageIOWizard.h"

namespace Ui {
class OverlayRolePage;
}

namespace imageiowiz {

class OverlayRolePage : public AbstractPage
{
  Q_OBJECT

public:
  explicit OverlayRolePage(QWidget *parent = 0);
  ~OverlayRolePage();

  void initializePage();
  bool validatePage();
  virtual bool isComplete() const;

private:
  Ui::OverlayRolePage *ui;
};

}

#endif // OVERLAYROLEPAGE_H
