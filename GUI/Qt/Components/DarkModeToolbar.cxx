#include "DarkModeToolbar.h"
#include <QEvent>
#include <QGuiApplication>
#include <QStyleOption>
#include <QPainter>

const char style_sheet_light[] = R"foo(
QToolButton
{
  min-width: 1ex;
  border-top: 1px solid rgb(192, 192, 192);
  border-bottom: 1px solid rgb(192, 192, 192);
  border-left: 1px;
  border-right: 1px;
  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgb(255, 255, 255), stop:0.0394089 rgb(235, 235, 235), stop:0.487685 rgb(215, 215, 215), stop:0.502463 rgb(215, 215, 215), stop:1 rgb(235, 235, 235));
}

QToolButton:checked, QToolButton:hover
{
  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgb(255, 255, 255), stop:0.0394089 rgb(214, 228, 245), stop:0.487685 rgb(193, 205, 221), stop:0.502463 rgb(182, 193, 207), stop:1 rgb(235, 228, 245));
  border-top: 1px solid rgb(192, 192, 192);
  border-bottom: 1px solid rgb(192, 192, 192);
}

DarkModeToolbar {
  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgb(255, 255, 255), stop:0.0394089 rgb(235, 235, 235), stop:0.487685 rgb(215, 215, 215), stop:0.502463 rgb(215, 215, 215), stop:1 rgb(235, 235, 235));
  border-top: 1px solid rgb(192, 192, 192);
  border-bottom: 1px solid rgb(192, 192, 192);
};
)foo";

DarkModeToolbar::DarkModeToolbar(QWidget *parent)
  : QWidget{parent}
{
}

bool DarkModeToolbar::event(QEvent *ev)
{
  if(ev->type() == QEvent::Paint)
    {
    Style darkMode = QGuiApplication::palette().color(QPalette::Window).valueF() < 0.5 ? DARK : LIGHT;
    if(darkMode != m_StyledForDarkMode)
      {
      QString ss(style_sheet_light);
      if(darkMode == DARK)
        {
        ss.replace("rgb(192, 192, 192)", "rgb(64, 64, 64)");
        ss.replace("rgb(215, 215, 215)", "rgb(155, 155, 155)");
        ss.replace("rgb(235, 235, 235)", "rgb(175, 175, 175)");
        ss.replace("rgb(255, 255, 255)", "rgb(195, 195, 195)");
        ss.replace("rgb(214, 228, 245)", "rgb(154, 168, 185)");
        ss.replace("rgb(193, 205, 221)", "rgb(133, 145, 161)");
        ss.replace("rgb(182, 193, 207)", "rgb(122, 133, 147)");
        ss.replace("rgb(235, 228, 245)", "rgb(155, 168, 185)");
        }
      this->setStyleSheet(ss);
      m_StyledForDarkMode = darkMode;
      }
    }

  return QWidget::event(ev);
  }

void DarkModeToolbar::paintEvent(QPaintEvent *)
{
  QStyleOption opt;
  opt.initFrom(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
