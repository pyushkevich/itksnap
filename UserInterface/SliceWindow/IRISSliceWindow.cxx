/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: IRISSliceWindow.cxx,v $
  Language:  C++
  Date:      $Date: 2010/06/28 18:45:08 $
  Version:   $Revision: 1.11 $
  Copyright (c) 2007 Paul A. Yushkevich
  
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
#include "IRISSliceWindow.h"

#include "GlobalState.h"
#include "OpenGLSliceTexture.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "UserInterfaceBase.h"
#include "AnnotationInteractionMode.h"
#include "CrosshairsInteractionMode.h"
#include "PolygonInteractionMode.h"
#include "PaintbrushInteractionMode.h"
#include "RegionInteractionMode.h"

#include <assert.h>
#include <stdio.h>
#include <cstdlib>
#include <cmath>
#include <iostream>

#include "itkImage.h"
#include "itkImageRegionIteratorWithIndex.h"

using namespace std;

IRISSliceWindow
::IRISSliceWindow(int id, UserInterfaceBase *parentUI, FLTKCanvas *canvas) 
: GenericSliceWindow(id, parentUI, canvas)
{
  // Initialize the interaction modes
  m_PolygonMode = new PolygonInteractionMode(this);
  m_RegionMode = new RegionInteractionMode(this);
  m_PaintbrushMode = new PaintbrushInteractionMode(this);
  m_AnnotationMode = new AnnotationInteractionMode(this);

  // Get a pointer to the drawing object
  m_PolygonDrawing = m_PolygonMode->m_Drawing;
  m_PolygonDrawing->SetFreehandFittingRate(parentUI->GetFreehandFittingRate());

  // Initialize polygon slice canvas to NULL
  m_PolygonSlice = NULL;

  // Register the interaction modes
  m_PolygonMode->Register();
  m_RegionMode->Register();
  m_PaintbrushMode->Register();
  m_AnnotationMode->Register();

}

IRISSliceWindow
::~IRISSliceWindow()
{
  delete m_PolygonMode;
  delete m_RegionMode;
  delete m_AnnotationMode;
  delete m_PaintbrushMode;
}

void 
IRISSliceWindow
::InitializeSlice(GenericImageData *imageData)
{
  // Call the parent's version of this method
  GenericSliceWindow::InitializeSlice(imageData);
  if(!imageData->IsMainLoaded())
	return;

  // Reset the polygon drawing interface
  m_PolygonDrawing->Reset();

  // Initialize the polygon drawing canvas
  itk::Size<2> imgSize;
  imgSize[0] = m_SliceSize(0);
  imgSize[1] = m_SliceSize(1);
  m_PolygonSlice = PolygonSliceType::New();
  m_PolygonSlice->SetRegions(imgSize);
  m_PolygonSlice->Allocate();
}

void
IRISSliceWindow
::DrawOverlays()
{
  // Call the parent's version of this method
  GenericSliceWindow::DrawOverlays();

  // Draw the polygon
  if(!m_ThumbnailIsDrawing)
    m_PolygonMode->OnDraw();

  // Draw the region of interest if selected
  if(IsInteractionModeAdded(m_RegionMode))
    m_RegionMode->OnDraw();

  // Draw the paintbrush stuff if selected
  if(IsInteractionModeAdded(m_PaintbrushMode) && !m_ThumbnailIsDrawing)
    m_PaintbrushMode->OnDraw();

  // Always draw annotations, whether active or not - but not in the thumbnail
  if(!m_ThumbnailIsDrawing)
    m_AnnotationMode->OnDraw();
}

void 
IRISSliceWindow
::EnterPolygonMode()
{
  EnterInteractionMode(m_PolygonMode);
}

void 
IRISSliceWindow
::EnterPaintbrushMode()
{
  EnterInteractionMode(m_PaintbrushMode);
}

void 
IRISSliceWindow
::EnterAnnotationMode()
{
  EnterInteractionMode(m_AnnotationMode);
}

void 
IRISSliceWindow
::EnterRegionMode()
{
  EnterInteractionMode(m_RegionMode);
}

bool 
IRISSliceWindow
::AcceptPolygon() 
{
  assert(m_IsRegistered && m_IsSliceInitialized);

  // Make sure we are in editing mode
  if (m_PolygonDrawing->GetState() != PolygonDrawing::EDITING_STATE) 
    return false;

#ifdef DRAWING_LOCK
  if (m_GlobalState->GetDrawingLock(id)) {
#endif /* DRAWING_LOCK */

    // VERY IMPORTANT - makes GL state current!
    m_Canvas->make_current(); 

    // Have the polygon drawing object render the polygon slice
    m_PolygonDrawing->AcceptPolygon(m_PolygonSlice);
      
    // take polygon rendered by polygon_drawing and merge with 
    // segmentation slice; send changes to voxel data set
    LabelType drawing_color = m_GlobalState->GetDrawingColorLabel();
    LabelType overwrt_color = m_GlobalState->GetOverWriteColorLabel();
    CoverageModeType mode = m_GlobalState->GetCoverageMode();

    // Get the current slice from the segmentation image
    LabelImageWrapper *seg = 
        m_Driver->GetCurrentImageData()->GetSegmentation();
    LabelImageWrapper::SlicePointer slice = seg->GetSlice(m_Id);
    slice->Update();

    // Create an iterator to iterate over the slice
    typedef itk::ImageRegionIteratorWithIndex<PolygonSliceType> PolygonIterator;
    PolygonIterator itPolygon(m_PolygonSlice,
                              m_PolygonSlice->GetLargestPossibleRegion());

    // Keep track of the number of pixels changed
    unsigned int nUpdates = 0;

    // Iterate
    for (; !itPolygon.IsAtEnd(); ++itPolygon)
    {
      // Get the current polygon pixel      
      PolygonSliceType::PixelType pxPolygon = itPolygon.Get();
      
      // Check for non-zero alpha of the pixel
      if((pxPolygon != 0) ^  m_GlobalState->GetPolygonInvert())
        {
        // Get the corresponding segmentation image pixel
        itk::Index<2> idx = itPolygon.GetIndex();
        LabelType pxLabel = slice->GetPixel(idx);

        // Check if we should be overriding that pixel
        if (mode == PAINT_OVER_ALL ||
            (mode == PAINT_OVER_ONE && pxLabel == overwrt_color) ||
            (mode == PAINT_OVER_COLORS && pxLabel != 0))
          {
          // Get the index into the image that we'll be updating
          Vector3f idxImageFloat = 
            MapSliceToImage(Vector3f(idx[0]+0.5f,idx[1]+0.5f,m_DisplayAxisPosition));

          // Convert to integer
          Vector3ui idxImage = to_unsigned_int(idxImageFloat);
          
          // Set the value of the pixel in segmentation image
          m_Driver->GetCurrentImageData()->SetSegmentationVoxel(
            idxImage, drawing_color);

          // Increment the counter
          nUpdates++;
          }
        }
      }

#ifdef DRAWING_LOCK
    m_GlobalState->ReleaseDrawingLock(m_Id);
  }
#endif /* DRAWING_LOCK */

  return (nUpdates > 0);
}

void 
IRISSliceWindow
::PastePolygon()
{
  assert(m_IsRegistered && m_IsSliceInitialized);

#ifdef DRAWING_LOCK
  if (m_GlobalState->GetDrawingLock(m_Id)) 
    {
#endif /* DRAWING_LOCK */

    if (m_PolygonDrawing->GetCachedPolygon()) 
      {
      m_PolygonDrawing->PastePolygon();
      m_Canvas->redraw();
      }

#ifdef DRAWING_LOCK
    } 
  else 
    {
    m_GlobalState->ReleaseDrawingLock(m_Id);
    }
#endif /* DRAWING_LOCK */
}

void 
IRISSliceWindow
::ClearPolygon()
{
  m_PolygonDrawing->Delete();
  m_Canvas->redraw();
}

void 
IRISSliceWindow
::DeleteSelectedPolygonPoints()
{
  m_PolygonDrawing->Delete();
  m_Canvas->redraw();
}

void 
IRISSliceWindow
::InsertPolygonPoints()
{
  m_PolygonDrawing->Insert();
  m_Canvas->redraw();
}



