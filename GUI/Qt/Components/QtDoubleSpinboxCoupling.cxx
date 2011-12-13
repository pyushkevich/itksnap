/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/

#include <QDoubleSpinBox>
#include <SNAPEvents.h>
#include "QtDoubleSpinboxCoupling.h"
#include "LatentITKEventNotifier.h"

QtDoubleSpinboxCoupling
::QtDoubleSpinboxCoupling(QDoubleSpinBox *widget, ModelType *model)
  : QObject(widget), m_Widget(widget), m_Model(model)
{
  // Update the values from the model
  updateWidgetFromModel();

  // Listen to value change events for this widget
  connect(widget, SIGNAL(valueChanged(double)), SLOT(onWidgetValueChanged(double)),
          Qt::DirectConnection);

  // Listen to value change events from the model
  LatentITKEventNotifier::connect(model, ValueChangedEvent(),
                                  this, SLOT(onModelUpdate(const EventBucket &)));

  LatentITKEventNotifier::connect(model, RangeChangedEvent(),
                                  this, SLOT(onModelUpdate(const EventBucket &)));

  m_Updating = false;
}

void
QtDoubleSpinboxCoupling
::onWidgetValueChanged(double value)
{
  if(!m_Updating)
    {
    double modelVal = m_Model->GetValue();
    if(modelVal != value && !vnl_math_isnan(modelVal))
      m_Model->SetValue(value);
    }
}

void
QtDoubleSpinboxCoupling
::updateWidgetFromModel()
{
  m_Updating = true;
  if(!m_Model->IsValueNull())
    {
    NumericValueRange<double> range = m_Model->GetRange();
    m_Widget->setMinimum(range.Minimum);
    m_Widget->setMaximum(range.Maximum);
    m_Widget->setSingleStep(range.StepSize);
    m_Widget->setValue(m_Model->GetValue());
    m_Widget->setSpecialValueText("");
    }
  else
    {
    m_Widget->setValue(m_Widget->minimum());
    m_Widget->setSpecialValueText(" ");
    }
  m_Updating = false;
}

void QtDoubleSpinboxCoupling
::onModelUpdate(const EventBucket &)
{
  this->updateWidgetFromModel();
}


