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

#ifndef QTLINEEDITCOUPLING_H
#define QTLINEEDITCOUPLING_H

#include "QtWidgetCoupling.h"
#include "SNAPQtCommon.h"
#include <iostream>
#include <iomanip>
#include <QLineEdit>

template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QLineEdit>
    : public WidgetValueTraitsBase<TAtomic, QLineEdit *>
{
public:

  virtual TAtomic GetValue(QLineEdit *w)
  {
    std::istringstream iss(to_utf8(w->text()));
    TAtomic value;
    iss >> value;
    return value;
  }

  virtual void SetValue(QLineEdit *w, const TAtomic &value)
  {
    std::ostringstream oss;
    oss << value;
    w->setText(from_utf8(oss.str()));
  }

  virtual void SetValueToNull(QLineEdit *w)
  {
    w->setText("");
  }

  virtual const char *GetSignal()
  {
    return SIGNAL(textChanged(const QString &));
  }
};

template<>
class DefaultWidgetValueTraits<std::string, QLineEdit>
    : public WidgetValueTraitsBase<std::string, QLineEdit *>
{
public:

  virtual std::string GetValue(QLineEdit *w)
  {
    return to_utf8(w->text());
  }

  virtual void SetValue(QLineEdit *w, const std::string &value)
  {
    w->setText(from_utf8(value));
  }

  virtual void SetValueToNull(QLineEdit *w)
  {
    w->setText("");
  }

  virtual const char *GetSignal()
  {
    return SIGNAL(textChanged(const QString &));
  }
};



/** Base class for traits that map between a numeric value and a text editor */
template <class TAtomic, class QLineEdit>
class FixedPrecisionRealToTextFieldWidgetTraits
    : public DefaultWidgetValueTraits<TAtomic, QLineEdit>
{
public:

  FixedPrecisionRealToTextFieldWidgetTraits(int precision)
    : m_Precision(precision) {}

  irisGetSetMacro(Precision, int)

  virtual void SetValue(QLineEdit *w, const TAtomic &value)
  {
    std::ostringstream oss;
    oss << std::setprecision(m_Precision) << value;
    w->setText(oss.str().c_str());
  }

protected:

  int m_Precision;
};


#endif // QTLINEEDITCOUPLING_H
