#include "QDarkModeToolButton.h"
#include <QEvent>
#include <QIcon>
#include <QGuiApplication>
#include <algorithm>

QDarkModeToolButton::QDarkModeToolButton(QWidget *parent)
  : QToolButton(parent)
{
  // Default mode is light
  isDark = false;
  iconKey = 0;
}

void QDarkModeToolButton::updateIconColor()
{
  // If the icon has changed since last time, we reset the dark value
  if(this->icon().cacheKey() != this->iconKey)
    {
    isDark = false;
    this->iconKey = this->icon().cacheKey();
    }

  bool updatedIsDark = QGuiApplication::palette().color(QPalette::Window).valueF() < 0.5;
  if(isDark != updatedIsDark)
    {
    QList<QSize> sizes = this->icon().availableSizes();

    if(sizes.length())
      {
      std::sort(sizes.begin(), sizes.end(), [] (const QSize &a, const QSize &b) { return a.width() > b.width(); });
      QImage img = this->icon().pixmap(sizes.back()).toImage();
      img.invertPixels(QImage::InvertRgb);
      this->setIcon(QIcon(QPixmap::fromImage(img)));
      this->iconKey = this->icon().cacheKey();
      }
    isDark = updatedIsDark;
    }
}

bool QDarkModeToolButton::event(QEvent *evt)
{
  if(evt->type() == QEvent::ApplicationPaletteChange || evt->type() == QEvent::Paint)
    updateIconColor();

  return QToolButton::event(evt);
}
