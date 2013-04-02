#ifndef QDOUBLESLIDER_H
#define QDOUBLESLIDER_H

#include <QSlider>

class QDoubleSlider : public QSlider
{
  Q_OBJECT
public:
  explicit QDoubleSlider(QWidget *parent = 0);

  double doubleMinimum()
  {
    return m_DoubleMin;
  }

  double doubleMaximum()
  {
    return m_DoubleMax;
  }

  double doubleSingleStep()
  {
    return m_DoubleStep;
  }

  void setDoubleMinimum(double value)
  {
    m_DoubleMin = value;
    updateRange();
  }

  void setDoubleMaximum(double value)
  {
    m_DoubleMax = value;
    updateRange();
  }

  void setDoubleSingleStep(double value)
  {
    m_DoubleStep = value;
    updateRange();
  }

  double doubleValue();

  void setDoubleValue(double x);

signals:

public slots:

private:
  double m_DoubleMin;
  double m_DoubleMax;
  double m_DoubleStep;
  double m_DoubleValue;

  int m_CorrespondingIntValue;

  void updateRange();

  void updateStep()
  {
    QSlider::setSingleStep((int)(1000 * m_DoubleStep / (m_DoubleMax - m_DoubleMin)));
  }
};

#endif // QDOUBLESLIDER_H
