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

#ifndef TAGLISTWIDGETCOUPLING_H
#define TAGLISTWIDGETCOUPLING_H

#include "QtWidgetCoupling.h"
#include "TagListWidget.h"
#include "SNAPQtCommon.h"
#include "TagList.h"

template<>
class DefaultWidgetValueTraits<TagList, TagListWidget>
    : public WidgetValueTraitsBase<TagList, TagListWidget *>
{
public:
  virtual TagList GetValue(TagListWidget *w)
  {
    // Get the tag strings from the widget
    const QStringList &tags = w->tags();
    TagList ret_list;
    foreach(const QString &tag, tags)
      ret_list.push_back(to_utf8(tag));

    return ret_list;
  }

  virtual void SetValue(TagListWidget *w, const TagList &value)
  {
    QStringList tags;
    for(TagList::const_iterator it = value.begin(); it != value.end(); ++it)
      tags.push_back(from_utf8(*it));

    w->setTags(tags);
  }

  virtual void SetValueToNull(TagListWidget *w)
  {
    w->clear();
  }

  virtual const char *GetSignal()
  {
    return SIGNAL(tagsEdited());
  }
};


#endif // TAGLISTWIDGETCOUPLING_H
