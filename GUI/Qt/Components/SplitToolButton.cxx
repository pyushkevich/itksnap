#include "SplitToolButton.h"
#include <QEvent>

SplitToolButton::SplitToolButton(QWidget *parent)
  : QToolButton(parent)
{
  UpdateStyle();
}

void SplitToolButton::changeEvent(QEvent *event)
{
  QToolButton::changeEvent(event);
  if(event->type() == QEvent::PaletteChange)
    UpdateStyle();
}

void SplitToolButton::showEvent(QShowEvent *event)
{
  // The popup mode and the menu are usually set up after the button is
  // created, so we check the style again before it is shown
  UpdateStyle();
  QToolButton::showEvent(event);
}

void SplitToolButton::UpdateStyle()
{
  // A style sheet can't darken a palette color, so the colors are worked out
  // here, like Fusion does it. In dark mode, the frame is made lighter than
  // the button so that it can be seen
  QPalette pal = this->palette();
  QColor button = pal.color(QPalette::Button);
  bool dark = pal.color(QPalette::Window).valueF() < 0.5;
  QColor pressed = dark ? button.lighter(160) : button.darker(115);
  QColor frame = dark ? button.lighter(200) : pal.color(QPalette::Mid);
  QColor hover_frame = dark ? button.lighter(280) : pal.color(QPalette::Dark);

  // Buttons that only show an icon get a bit less room on the sides
  int hpad = (this->toolButtonStyle() == Qt::ToolButtonIconOnly && !this->icon().isNull()) ? 2 : 4;

  QString css = QString(
    "QToolButton {"
    "  border: 1px solid %1;"
    "  border-radius: 4px;"
    "  background-color: %2;"
    "  color: palette(button-text);"
    "  padding: 1px %5px;"
    "}"
    "QToolButton:hover { border-color: %3; }"
    "QToolButton:pressed, QToolButton:checked { background-color: %4; }")
    .arg(frame.name(), button.name(), hover_frame.name(), pressed.name())
    .arg(hpad);

  // The menu arrow goes inside the frame, with room made for it on the right.
  // The arrow needs an explicit size, otherwise the @2x image is drawn at
  // twice the size on a Retina screen
  if(this->popupMode() == QToolButton::MenuButtonPopup)
    {
    css += QString(
      "QToolButton { padding-right: 16px; }"
      "QToolButton::menu-button {"
      "  subcontrol-origin: padding;"
      "  subcontrol-position: center right;"
      "  border: none;"
      "  border-left: 1px solid %1;"
      "  border-top-right-radius: 3px;"
      "  border-bottom-right-radius: 3px;"
      "  width: 14px;"
      "}"
      "QToolButton::menu-button:pressed { background-color: %2; }"
      "QToolButton::menu-arrow { image: url(:/root/menu-arrow_8.png); width: 8px; height: 4px; }")
      .arg(frame.name(), pressed.name());
    }

  // Setting the style sheet should not happen again and again
  if(this->styleSheet() != css)
    this->setStyleSheet(css);
}
