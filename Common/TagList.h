/*=========================================================================

  Program:   ITK-SNAP
  Module:    TagList.h
  Language:  C++
  Date:      March 2018

  Copyright (c) 2018 Paul A. Yushkevich

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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef TAGS_H
#define TAGS_H

#include <list>
#include <string>

/**
 * @brief A list of string tags
 */
class TagList : public std::list<std::string>
{
public:
  typedef std::list<std::string> Superclass;

  TagList();

  /** Check if a tag is present */
  bool Contains(const std::string &tag) const;

  /** Add a tag, but only if it is not already present. Returns true if actually added */
  bool AddTag(const std::string &tag);

  /** Removes a tag if present. Returns true if tag was actually removed */
  bool RemoveTag(const std::string &tag);

  std::string ToString() const;

protected:
};

#endif // TAGS_H
