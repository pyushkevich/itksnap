#ifndef DARKMODETOOLBAR_H
#define DARKMODETOOLBAR_H

#include <QWidget>

class DarkModeToolbar : public QWidget
{
  Q_OBJECT
public:
  explicit DarkModeToolbar(QWidget *parent = nullptr);
  virtual bool event(QEvent *ev) override;
  virtual void paintEvent(QPaintEvent *) override;

signals:

protected:
  enum Style { NONE, DARK, LIGHT };
  Style m_StyledForDarkMode = NONE;
};

#endif // DARKMODETOOLBAR_H
