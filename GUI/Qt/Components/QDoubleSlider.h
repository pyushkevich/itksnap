#ifndef QDOUBLESLIDER_H
#define QDOUBLESLIDER_H

#include <QSlider>

class QDoubleSlider : public QSlider
{
  Q_OBJECT
public:
  explicit QDoubleSlider(QWidget *parent = 0);

  double minimum()
  {
    return m_DoubleMin;
  }

  double maximum()
  {
    return m_DoubleMax;
  }

  double singleStep()
  {
    return m_DoubleStep;
  }

  void setMinimum(double value)
  {
    m_DoubleMin = value;
    updateStep();
  }

  void setMaximum(double value)
  {
    m_DoubleMax = value;
    updateStep();
  }

  void setSingleStep(double value)
  {
    m_DoubleStep = value;
    updateStep();
  }

  double value() const
  {
    return QSlider::value() * 0.001 * (m_DoubleMax-m_DoubleMin) + m_DoubleMin;
  }

  void setValue(double x)
  {
    int v = (int) 1000 * (x - m_DoubleMin) / (m_DoubleMax-m_DoubleMin);
    QSlider::setValue(v);
  }

signals:

public slots:

private:
  double m_DoubleMin;
  double m_DoubleMax;
  double m_DoubleStep;

  void updateStep()
  {
    QSlider::setSingleStep(1000 * m_DoubleStep / (m_DoubleMax - m_DoubleMin));
  }
};

#endif // QDOUBLESLIDER_H
