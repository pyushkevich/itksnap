#ifndef SPLITTOOLBUTTON_H
#define SPLITTOOLBUTTON_H

#include <QToolButton>

/**
  A tool button that draws itself with a style sheet, so that it looks the
  same in all styles and in dark mode. Its main use is as a split button: when
  it has a menu and the popup mode is MenuButtonPopup, the menu arrow is drawn
  inside the frame of the button, separated by a thin line. The Fusion style
  draws that arrow as a separate button, which looks odd next to other
  buttons.

  Without a menu it is just a plain button with the same look, so it can be
  placed next to split buttons (like the cursor Back and Forward buttons next
  to "zoom to fit" in the slice views).

  The colors are taken from the palette, and the style sheet is rebuilt when
  the palette changes. In dark mode, the button gets lighter rather than
  darker when it is pressed.
  */
class SplitToolButton : public QToolButton
{
  Q_OBJECT

public:
  explicit SplitToolButton(QWidget *parent = nullptr);

protected:
  void changeEvent(QEvent *event) override;
  void showEvent(QShowEvent *event) override;

  // Build the style sheet from the palette and the popup mode
  void UpdateStyle();
};

#endif // SPLITTOOLBUTTON_H
