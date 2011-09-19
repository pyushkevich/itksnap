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

#ifndef QTDOUBLESPINBOXCOUPLING_H
#define QTDOUBLESPINBOXCOUPLING_H

#include <QObject>
#include <EditableNumericValueModel.h>

class QDoubleSpinBox;

class QtDoubleSpinboxCoupling : public QObject
{
  Q_OBJECT

public:

  // The model that couples with this object
  typedef AbstractEditableNumericValueModel<double> ModelType;

  explicit QtDoubleSpinboxCoupling(QDoubleSpinBox *widget, ModelType *model);

public slots:

  void onWidgetValueChanged(double value);

protected:

  void updateWidgetFromModel();

  QDoubleSpinBox *m_Widget;
  ModelType *m_Model;

  bool m_SettingValue;
};

#endif // QTDOUBLESPINBOXCOUPLING_H