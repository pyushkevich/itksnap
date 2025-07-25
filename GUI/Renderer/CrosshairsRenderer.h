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

#ifndef CROSSHAIRSRENDERER_H
#define CROSSHAIRSRENDERER_H

#include "SNAPCommon.h"
#include "GenericSliceRenderer.h"


class OrthogonalSliceCursorNavigationModel;

class CrosshairsRenderer : public SliceRendererDelegate
{
public:
  irisITKObjectMacro(CrosshairsRenderer, SliceRendererDelegate)

  void RenderOverTiledLayer(AbstractRenderContext *context,
                            ImageWrapperBase         *base_layer,
                            const SubViewport        &vp) override;

  irisGetSetMacro(Model, GenericSliceModel *)

protected:
  CrosshairsRenderer() {}
  virtual ~CrosshairsRenderer() {}

  GenericSliceModel *m_Model;
};

#endif // CROSSHAIRSRENDERER_H
