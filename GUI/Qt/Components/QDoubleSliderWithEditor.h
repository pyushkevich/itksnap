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

public slots:

  void sliderValueChanged(int);
  void spinnerValueChanged(double);

signals:
  void valueChanged(double);

private:

  void updateSliderFromSpinner();

  Ui::QDoubleSliderWithEditor *ui;

  bool m_IgnoreSliderEvent, m_IgnoreSpinnerEvent;
};

#endif // QDOUBLESLIDERWITHEDITOR_H
