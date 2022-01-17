#include "DarkModeToolbar.h"
#include <QEvent>
#include <QGuiApplication>
#include <QStyleOption>
#include <QPainter>

const char style_sheet_light[] =
    "QToolButton\n"
    "{\n"
    "  min-width: 1ex;\n"
    "  border-top: 1px solid rgb(192, 192, 192);\n"
    "  border-bottom: 1px solid rgb(192, 192, 192);\n"
    "  border-left: 1px;\n"
    "  border-right: 1px;\n"
    "  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(255, 255, 255, 255), stop:0.0394089 rgba(235, 235, 235, 255), stop:0.487685 rgba(216, 216, 216, 255), stop:0.502463 rgba(215, 215, 215, 255), stop:1 rgba(235, 235, 235, 255));\n"
    "}\n"
    "\n"
    "QToolButton:checked, QToolButton:hover\n"
    "{\n"
    "  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(255, 255, 255, 255), stop:0.0394089 rgba(214, 228, 245, 255), stop:0.487685 rgba(193, 205, 221, 255), stop:0.502463 rgba(182, 193, 207, 255), stop:1 rgba(235, 228, 245, 255));\n"
    "  padding:0px;\n"
    "}\n"
    "\n"
    "DarkModeToolbar {\n"
    "  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(255, 255, 255, 255), stop:0.0394089 rgba(235, 235, 235, 255), stop:0.487685 rgba(216, 216, 216, 255), stop:0.502463 rgba(215, 215, 215, 255), stop:1 rgba(235, 235, 235, 255));\n"
    "  border-top: 1px solid rgb(192, 192, 192);\n"
    "  border-bottom: 1px solid rgb(192, 192, 192);\n"
    "}\n";

const char style_sheet_dark[] =
    "QToolButton\n"
    "{\n"
    "  min-width: 1ex;\n"
    "  border-top: 1px solid rgb(64, 64, 64);\n"
    "  border-bottom: 1px solid rgb(64, 64, 64);\n"
    "  border-left: 1px;\n"
    "  border-right: 1px;\n"
    "  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(185, 185, 185, 255), stop:0.0394089 rgba(175, 175, 175, 255), stop:0.487685 rgba(156, 156, 156, 255), stop:0.502463 rgba(155, 155, 155, 255), stop:1 rgba(175, 175, 175, 255));\n"
    "}\n"
    "\n"
    "QToolButton:checked, QToolButton:hover\n"
    "{\n"
    "  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(195, 195, 195, 255), stop:0.0394089 rgba(154, 168, 185, 255), stop:0.487685 rgba(133, 145, 161, 255), stop:0.502463 rgba(122, 133, 147, 255), stop:1 rgba(155, 168, 185, 255));\n"
    "  padding:0px;\n"
    "}\n"
    "\n"
    "DarkModeToolbar {\n"
    "  background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(185, 185, 185, 255), stop:0.0394089 rgba(175, 175, 175, 255), stop:0.487685 rgba(156, 156, 156, 255), stop:0.502463 rgba(155, 155, 155, 255), stop:1 rgba(175, 175, 175, 255));\n"
    "  border-top: 1px solid rgb(64, 64, 64);\n"
    "  border-bottom: 1px solid rgb(64, 64, 64);\n"
    "}\n";

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
      this->setStyleSheet(QString(darkMode == DARK ? style_sheet_dark : style_sheet_light));
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
