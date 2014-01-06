#ifndef QDOUBLESLIDERWITHEDITOR_H
#define QDOUBLESLIDERWITHEDITOR_H

#include <QWidget>

namespace Ui {
  class QDoubleSliderWithEditor;
}

class QDoubleSliderWithEditor : public QWidget
{
  Q_OBJECT

public:

  Q_PROPERTY(double value
             READ value
             WRITE setValue
             NOTIFY valueChanged)

  Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
  Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
  Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)

  explicit QDoubleSliderWithEditor(QWidget *parent = 0);
  ~QDoubleSliderWithEditor();

  double value();
  void setValue(double newval);

  void setValueToNull();

  double minimum();
  double maximum();
  double singleStep();
  void setMinimum(double);
  void setMaximum(double);
  void setSingleStep(double);
  void setOrientation(Qt::Orientation) {}

  /** Whether the slider uses discrete steps (in units of SingleStep) or
   * a continuous range of values */
  void setForceDiscreteSteps(bool useDiscreteSteps);

public slots:

  void sliderValueChanged(int);
  void spinnerValueChanged(double);
  void stepUp();
  void stepDown();

signals:
  void valueChanged(double);

private:

  void updateSliderFromSpinner();

  Ui::QDoubleSliderWithEditor *ui;

  bool m_IgnoreSliderEvent, m_IgnoreSpinnerEvent;
  bool m_ForceDiscreteSteps;
};

#endif // QDOUBLESLIDERWITHEDITOR_H
