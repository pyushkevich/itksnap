#include "QDoubleSlider.h"

QDoubleSlider::QDoubleSlider(QWidget *parent) :
  QSlider(parent)
{
  QSlider::setMinimum(0);
  QSlider::setMaximum(1000);
  m_DoubleMin = 0.0;
  m_DoubleMax = 1.0;
  m_DoubleStep = 0.01;
}


