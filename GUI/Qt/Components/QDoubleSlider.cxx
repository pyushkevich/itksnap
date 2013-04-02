#include "QDoubleSlider.h"
#include <cmath>

QDoubleSlider::QDoubleSlider(QWidget *parent) :
  QSlider(parent)
{
  m_DoubleMin = 0.0;
  m_DoubleMax = 1.0;
  m_DoubleStep = 0.01;
  updateRange();
}

void QDoubleSlider::updateRange()
{
  int mymax = ceil((m_DoubleMax - m_DoubleMin) / m_DoubleStep);
  this->setMinimum(0);
  this->setMaximum(mymax);
  this->setSingleStep(1);

  this->setDoubleValue(m_DoubleValue);
}

void QDoubleSlider::setDoubleValue(double x)
{
  m_DoubleValue = x;
  double t = (m_DoubleValue - m_DoubleMin) / (m_DoubleMax - m_DoubleMin);
  t = std::max(0.0, std::min(1.0, t));
  int p = (int)(0.5 + this->maximum() * t);
  if(this->value() != p)
    this->setValue(p);
  m_CorrespondingIntValue = p;
}

double QDoubleSlider::doubleValue()
{
  if(this->value() != m_CorrespondingIntValue)
    {
    double t = this->value() * 1.0 / this->maximum();
    m_DoubleValue = m_DoubleMin + t * (m_DoubleMax - m_DoubleMin);
    m_CorrespondingIntValue = this->value();
    }

  return m_DoubleValue;
}


