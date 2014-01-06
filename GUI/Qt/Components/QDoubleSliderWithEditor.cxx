#include "QDoubleSliderWithEditor.h"
#include "ui_QDoubleSliderWithEditor.h"
#include <cmath>

QDoubleSliderWithEditor::QDoubleSliderWithEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QDoubleSliderWithEditor)
{
  ui->setupUi(this);

  ui->slider->setMinimum(0);
  ui->slider->setMaximum(1000000);
  ui->slider->setSingleStep(0);

  // When the value in the slider changes, we want to update the spinbox
  connect(ui->slider, SIGNAL(valueChanged(int)),
          this, SLOT(sliderValueChanged(int)));
  connect(ui->spinbox, SIGNAL(valueChanged(double)),
          this, SLOT(spinnerValueChanged(double)));

  m_IgnoreSliderEvent = false;
  m_IgnoreSpinnerEvent = false;
  m_ForceDiscreteSteps = true;
}

QDoubleSliderWithEditor::~QDoubleSliderWithEditor()
{
  delete ui;
}

void QDoubleSliderWithEditor::setValue(double newval)
{
  // Set the value in the spinbox
  if(newval != ui->spinbox->value())
    {
    m_IgnoreSpinnerEvent = true;
    ui->spinbox->setValue(newval);
    ui->spinbox->setSpecialValueText("");
    m_IgnoreSpinnerEvent = false;
    this->updateSliderFromSpinner();
    }
}

double QDoubleSliderWithEditor::value()
{
  return ui->spinbox->value();
}

void QDoubleSliderWithEditor::updateSliderFromSpinner()
{
  // Set the value of the slider proportionally to the range of the spinner
  double a = ui->spinbox->minimum();
  double b = ui->spinbox->maximum();
  double v = ui->spinbox->value();

  double r = (v - a) / (b - a);
  int vs = (int) (ui->slider->maximum() * r);

  m_IgnoreSliderEvent = true;
  if(vs != ui->slider->value())
    ui->slider->setValue(vs);
  m_IgnoreSliderEvent = false;
}

double QDoubleSliderWithEditor::minimum()
{
  return ui->spinbox->minimum();
}

double QDoubleSliderWithEditor::maximum()
{
  return ui->spinbox->maximum();
}

double QDoubleSliderWithEditor::singleStep()
{
  return ui->spinbox->singleStep();
}

void QDoubleSliderWithEditor::setMinimum(double x)
{
  ui->spinbox->setMinimum(x);
  this->updateSliderFromSpinner();
}

void QDoubleSliderWithEditor::setMaximum(double x)
{
  ui->spinbox->setMaximum(x);
  this->updateSliderFromSpinner();
}

void QDoubleSliderWithEditor::setSingleStep(double x)
{
  ui->spinbox->setSingleStep(x);
}

void QDoubleSliderWithEditor::setForceDiscreteSteps(bool useDiscreteSteps)
{
  m_ForceDiscreteSteps = useDiscreteSteps;
}

void QDoubleSliderWithEditor::sliderValueChanged(int valslider)
{
  if(!m_IgnoreSliderEvent)
    {
    // We need to map the integer value into the closest acceptable by the spinbox
    double r = valslider * 1.0 / ui->slider->maximum();
    double a = ui->spinbox->minimum();
    double b = ui->spinbox->maximum();
    double v;

    // If necessary, round the value using singleStep
    if(m_ForceDiscreteSteps)
      {
      double step = ui->spinbox->singleStep();
      double t = 0.5 * step + (b - a) * r;
      v = a + (t - std::fmod(t, step));
      }
    else
      {
      v = a + (b - a) * r;
      }

    // Set the value in the spinner and slider
    this->setValue(v);

    // Invoke the event
    emit valueChanged(ui->spinbox->value());
    }
}

void QDoubleSliderWithEditor::spinnerValueChanged(double value)
{
  // This is very simple, we just stick the value into the slider
  if(!m_IgnoreSpinnerEvent)
    {
    this->updateSliderFromSpinner();
    emit valueChanged(value);
    }
}

void QDoubleSliderWithEditor::stepUp()
{
  ui->spinbox->stepUp();
}

void QDoubleSliderWithEditor::stepDown()
{
  ui->spinbox->stepDown();
}

void QDoubleSliderWithEditor::setValueToNull()
{
  // First, set the value to minimum
  m_IgnoreSliderEvent = true;
  m_IgnoreSpinnerEvent = true;
  ui->slider->setValue(ui->slider->minimum());
  ui->spinbox->setValue(ui->spinbox->minimum());
  ui->spinbox->setSpecialValueText(" ");
  m_IgnoreSliderEvent = false;
  m_IgnoreSpinnerEvent = false;
}
