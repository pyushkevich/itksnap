/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: GlobalState.cxx,v $
  Language:  C++
  Date:      $Date: 2009/06/16 04:55:45 $
  Version:   $Revision: 1.8 $
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
#include "GlobalState.h"


GlobalState
::GlobalState() 
{
  m_GreyFileExtension = NULL;
  m_DrawingColorLabel = 1;
  m_OverWriteColorLabel = 0;
  m_CrosshairsPosition[0] = 0;
  m_CrosshairsPosition[1] = 0;
  m_CrosshairsPosition[2] = 0;
  m_ToolbarMode = CROSSHAIRS_MODE;
  m_CoverageMode = PAINT_OVER_ALL;
  m_UpdateSliceFlag = 1;
  m_InterpolateGrey = false;
  m_InterpolateSegmentation = false;
  m_PolygonInvert = false;
  m_LockHeld = 0;
  m_LockOwner = 0;
  m_SegmentationAlpha = 0;

  // SNAP is off initially
  m_SNAPActive = false;

  // Presets
  m_SpeedColorMapInRegionMode = COLORMAP_BLUE_BLACK_WHITE;
  m_SpeedColorMapInEdgeMode = COLORMAP_BLACK_BLACK_WHITE;

  // Snake stuff:
  m_SpeedValid = false;
  m_ShowSpeed = false;
  m_IsValidROI = false;
  m_ShowROI = false;
  m_DraggingROI = false;
  m_SnakeMode = EDGE_SNAKE;
  m_SnakeActive = false;

  m_SpeedViewZero = false;
  m_SnakeParameters = SnakeParameters::GetDefaultEdgeParameters();
  m_ThresholdSettings = ThresholdSettings::MakeDefaultSettingsWithoutImage();
  m_EdgePreprocessingSettings = EdgePreprocessingSettings::MakeDefaultSettings();

  // Default preview modes: enabled for in-out, disabled for edges (too slow)
  m_ShowPreprocessedEdgePreview = false;
  m_ShowPreprocessedInOutPreview = true;

  // The preview is not currently valid
  m_SpeedPreviewValid = false;

  // Bubbles
  m_ActiveBubble = -1;

  // Set paintbrush defaults
  m_PaintbrushSettings.radius = 4;
  m_PaintbrushSettings.mode = PAINTBRUSH_RECTANGULAR;
  m_PaintbrushSettings.flat = true;
  m_PaintbrushSettings.isotropic = false;
  m_PaintbrushSettings.chase = false;
  m_PaintbrushSettings.watershed.level = 0.2;
  m_PaintbrushSettings.watershed.smooth_iterations = 15;

  // Set annotation defaults
  m_AnnotationSettings.shownOnAllSlices = false;

}

GlobalState
::~GlobalState() 
{
  if(m_GreyFileExtension != NULL)
    delete [] m_GreyFileExtension;
}

#ifdef DRAWING_LOCK

// TODO: What's this for?
// mutual exclusion on the drawing lock - only one window can draw at a time
int 
GlobalState
::GetDrawingLock( short windowID ) 
{
  if ((!m_LockHeld) || (m_LockOwner == windowID)) 
    {
    m_LockHeld = 1;
    m_LockOwner = windowID;
    return 1;
    }
  return 0;
}

int 
GlobalState
::ReleaseDrawingLock(short windowID) 
{
  if (m_LockHeld && (m_LockOwner != windowID)) 
    {
    cerr << "hmph: Cannot ReleaseDrawingLock(): not lock owner" << endl;
    return 0;
    }
  m_LockHeld = 0;
  m_LockOwner = 0;
  return 1;
}

#endif /* DRAWING_LOCK */

/**
 * Pulls the extension off of a filename and
 * saves it for future use in saving preproc
 * data
 *
 * PRE: none
 * POST: if fname is valid, m_GreyFileExtension will
 * have the file extension (including leading
 * period) of the file
 */
void 
GlobalState
::SetGreyExtension(const char *fname)
{
  const char * basename;
  // Make sure fname is valid
  if(fname != NULL)
    {

    // Get rid of any currently saved
    // extension
    if(m_GreyFileExtension != NULL)
      {                                                 
      delete [] m_GreyFileExtension;
      m_GreyFileExtension = NULL;
      }

    // Get rid of all path information
    // TODO: This is platform specific!!!
    basename = strrchr(fname, '/');
    if(basename)
      {
      basename++;
      }
    else
      {
      basename = fname;
      }

    // Find the first period and count everything
    // to the right of it as path.  Allows for
    // multiple periods (ie .hdr.gz)
    basename = strchr(basename, '.');
    if(basename)
      {
      m_GreyFileExtension = new char[strlen(basename) + 1];
      strcpy(m_GreyFileExtension, basename);
      }
    }
}

/**
 * Returns the extension for the grey file,
 * user is responsible for deallocating the
 * memory for ext
 * PRE: ext is NULL or dynamically allocated array
 * POST: ext points to a null terminated string
 * which contains the extension in m_GreyFileExtension
 * or ext is NULL if m_GreyFileExtension is empty
 */
void 
GlobalState
::GetGreyExtension(char *& ext)
{
  // Get rid of any data in ext
  if(ext != NULL)
    delete [] ext;

  // Copy m_GreyFileExtension into ext
  if(m_GreyFileExtension == NULL)
    ext = NULL;
  else
    {
    ext = new char[strlen(m_GreyFileExtension) + 1];
    strcpy(ext, m_GreyFileExtension);
    }
}

/** Set the colormap in current preprocessing mode*/
void 
GlobalState
::SetSpeedColorMap(ColorMapPreset xPreset)
{
  if(m_SnakeMode == EDGE_SNAKE)
    SetSpeedColorMapInEdgeMode(xPreset);
  else
    SetSpeedColorMapInRegionMode(xPreset);
}

/** Get the colormap in current preprocessing mode*/
ColorMapPreset 
GlobalState
::GetSpeedColorMap()
{
  return (m_SnakeMode == EDGE_SNAKE) ?
    GetSpeedColorMapInEdgeMode() : GetSpeedColorMapInRegionMode();
}

