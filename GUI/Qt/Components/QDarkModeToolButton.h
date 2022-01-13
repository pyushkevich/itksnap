#ifndef QDARKMODETOOLBUTTON_H
#define QDARKMODETOOLBUTTON_H

#include <QToolButton>

class QDarkModeToolButton : public QToolButton
{
  Q_OBJECT
public:
  explicit QDarkModeToolButton(QWidget *parent = 0);

  virtual bool event(QEvent *evt) override;

protected:

  bool isDark;
  qint64 iconKey;
  void updateIconColor();
};

#endif // QDARKMODETOOLBUTTON_H
