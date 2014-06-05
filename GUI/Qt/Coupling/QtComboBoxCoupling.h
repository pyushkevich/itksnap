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

#ifndef QTCOMBOBOXCOUPLING_H
#define QTCOMBOBOXCOUPLING_H

#include <QtWidgetCoupling.h>
#include "SNAPQtCommon.h"
#include <ColorLabel.h>
#include <QComboBox>

// This is to allow the code below to work with DrawOverFilter
Q_DECLARE_METATYPE(DrawOverFilter)

/**
  Default traits for mapping a numeric value (or any sort of key, actually)
  to a row in a combo box
  */
template <class TAtomic>
class DefaultWidgetValueTraits<TAtomic, QComboBox>
    : public WidgetValueTraitsBase<TAtomic, QComboBox *>
{
public:
  // Get the Qt signal that the widget fires when its value has changed. The
  // value here is the selected item in the combo box.
  const char *GetSignal()
  {
    return SIGNAL(currentIndexChanged(int));
  }

  TAtomic GetValue(QComboBox *w)
  {
    int index = w->currentIndex();
    QVariant id = w->itemData(index);
    return id.value<TAtomic>();
  }

  void SetValue(QComboBox *w, const TAtomic &value)
  {
    // We have to actually find the item. I looked up the Qt findItem
    // method and it seems quite involved and also quite long. So we
    // just do our own loop
    for(int i = 0; i < w->count(); i++)
      {
      if(value == w->itemData(i).value<TAtomic>())
        {
        w->setCurrentIndex(i);
        return;
        }
      }
    w->setCurrentIndex(-1);
  }

  void SetValueToNull(QComboBox *w)
  {
    w->setCurrentIndex(-1);
  }
};

/**
  These are the row traits for adding and updating rows in combo boxes. This
  class is further parameterized by the class TItemDesriptionTraits, which is
  used to obtain the text and icon information from the value/description pairs
  provided by the model.
  */
template <class TAtomic, class TDesc, class TItemDesriptionTraits>
class TextAndIconComboBoxRowTraits
{
public:
  static void removeAll(QComboBox *w)
  {
    w->clear();
  }

  static int getNumberOfRows(QComboBox *w)
  {
    return w->count();
  }

  static TAtomic getValueInRow(QComboBox *w, int i)
  {
    return w->itemData(i).value<TAtomic>();
  }

  static void appendRow(QComboBox *w, TAtomic label, const TDesc &desc)
  {
    // The description
    QString text = TItemDesriptionTraits::GetText(label, desc); // QString text(cl.GetLabel());

    // The icon
    QIcon icon = TItemDesriptionTraits::GetIcon(label, desc);

    // The icon signature - a value that can be used to check if the icon has changed
    QVariant iconSig = TItemDesriptionTraits::GetIconSignature(label, desc);

    // Icon based on the color
    w->addItem(icon, text, QVariant::fromValue(label));
    w->setItemData(w->count()-1, iconSig, Qt::UserRole + 1);
  }

  static void updateRowDescription(QComboBox *w, int index, const TDesc &desc)
  {
    // The current value
    TAtomic label = w->itemData(index).value<TAtomic>();

    // Get the properies and compare them to the color label
    QVariant currentIconSig = w->itemData(index, Qt::UserRole + 1);
    QVariant newIconSig = TItemDesriptionTraits::GetIconSignature(label, desc);

    if(currentIconSig != newIconSig)
      {
      QIcon ic = TItemDesriptionTraits::GetIcon(label, desc);
      w->setItemIcon(index, ic);
      w->setItemData(index, newIconSig, Qt::UserRole + 1);
      }

    QString currentText = w->itemText(index);
    QString newText = TItemDesriptionTraits::GetText(label, desc);

    if(currentText != newText)
      {
      w->setItemText(index, newText);
      }
  }
};

template<class TAtomic>
class StringRowDescriptionTraits
{
public:
  static QString GetText(TAtomic label, const std::string &text)
  {
    return QString(text.c_str());
  }

  static QIcon GetIcon(TAtomic label, const std::string &text)
  {
    return QIcon();
  }

  static QVariant GetIconSignature(TAtomic label, const std::string &text)
  {
    return QVariant(0);
  }
};

// TODO: this stuff should be replaced by coupling with the abstract item model
// Specific traits for filling drawing color label combos
class DrawingColorRowDescriptionTraits
{
public:

  static QString GetText(LabelType label, const ColorLabel &cl)
  {
    return GetTitleForColorLabel(cl);
  }

  static QIcon GetIcon(LabelType label, const ColorLabel &cl)
  {
    QBrush brush = GetBrushForColorLabel(cl);
    return CreateColorBoxIcon(16, 16, brush);
  }

  static QVariant GetIconSignature(LabelType label, const ColorLabel &cl)
  {
    return QColor(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2));
  }
};

// Specific traits for filling draw-over color label combos
class DrawOverFilterRowDescriptionTraits
{
public:

  static QString GetText(DrawOverFilter filter, const ColorLabel &cl)
  {
    return GetTitleForDrawOverFilter(filter, cl);
  }

  static QIcon GetIcon(DrawOverFilter filter, const ColorLabel &cl)
  {
    QBrush brush = GetBrushForDrawOverFilter(filter, cl);
    return CreateColorBoxIcon(16, 16, brush);
  }

  static QVariant GetIconSignature(DrawOverFilter filter, const ColorLabel &cl)
  {
    if(filter.CoverageMode == PAINT_OVER_ONE)
      return DrawingColorRowDescriptionTraits::GetIconSignature(filter.DrawOverLabel, cl);
    else
      return QVariant(filter.CoverageMode);
  }
};

/**
  Use template specialization to generate default traits based on the model
  */
template <class TAtomic, class TDesc>
class DefaultComboBoxRowTraits
{
};

template <>
class DefaultComboBoxRowTraits<LabelType, ColorLabel>
    : public TextAndIconComboBoxRowTraits<LabelType, ColorLabel, DrawingColorRowDescriptionTraits>
{
};

template <>
class DefaultComboBoxRowTraits<DrawOverFilter, ColorLabel>
    : public TextAndIconComboBoxRowTraits<DrawOverFilter, ColorLabel, DrawOverFilterRowDescriptionTraits>
{
};

template<class TAtomic>
class DefaultComboBoxRowTraits<TAtomic, std::string>
    : public TextAndIconComboBoxRowTraits<TAtomic, std::string, StringRowDescriptionTraits<TAtomic> >
{
};


// Define the defaults
template <class TDomain>
class DefaultWidgetDomainTraits<TDomain, QComboBox>
    : public ItemSetWidgetDomainTraits<
        TDomain, QComboBox,
        DefaultComboBoxRowTraits<typename TDomain::ValueType,
                                 typename TDomain::DescriptorType> >
{
};

// Define the behavior with trivial domain. In this case, we assume that the combo
// box was set up in the GUI to have userdata matching the items in the model
template <>
class DefaultWidgetDomainTraits<TrivialDomain, QComboBox>
    : public WidgetDomainTraitsBase<TrivialDomain, QComboBox *>
{
public:
  virtual void SetDomain(QComboBox *w, const TrivialDomain &domain) {}
  virtual TrivialDomain GetDomain(QComboBox *w) { return TrivialDomain(); }
};


#endif // QTCOMBOBOXCOUPLING_H
