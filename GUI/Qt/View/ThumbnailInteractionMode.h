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

#ifndef THUMBNAILINTERACTIONMODE_H
#define THUMBNAILINTERACTIONMODE_H

#include <SliceWindowInteractionDelegateWidget.h>

class OrthogonalSliceCursorNavigationModel;

class ThumbnailInteractionMode : public SliceWindowInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit ThumbnailInteractionMode(GenericSliceView *parent = 0);

  void SetModel(OrthogonalSliceCursorNavigationModel *model);

  void mousePressEvent(QMouseEvent *ev);
  void mouseMoveEvent(QMouseEvent *);
  void mouseReleaseEvent(QMouseEvent *);
  void contextMenuEvent(QContextMenuEvent *ev);
  void enterEvent(QEvent *);
  void leaveEvent(QEvent *);

public slots:

  void onContextMenuRequested(const QPoint &pt);

private:

  OrthogonalSliceCursorNavigationModel *m_Model;

  bool m_PanFlag;
};

#endif // THUMBNAILINTERACTIONMODE_H
