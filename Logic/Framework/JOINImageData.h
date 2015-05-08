/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPImageData.h,v $
  Language:  C++
  Date:      $Date: 2009/01/23 20:09:38 $
  Version:   $Revision: 1.4 $
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
#ifndef __JOINImageData_h_
#define __JOINImageData_h_

#include "SNAPCommon.h"

#include "GenericImageData.h"
#include "itkImageAdaptor.h"
#include "itkJoinCopyFilter.h"

/**
 * \class JOINImageData
 * \brief Wrapper around the JOIN automatic segmentation pipelines.
 *
 * This class encapsulates several images used in the JOIN application
 */
class JOINImageData : public GenericImageData 
{
public:
  irisITKObjectMacro(JOINImageData, GenericImageData)

  // The type of the internal level set image
  typedef Superclass::AnatomicImageType               AnatomicImageType;
  typedef JsrcImageWrapper::ImageType                     JsrcImageType;
  typedef JdstImageWrapper::ImageType                     JdstImageType;
  typedef WsrcImageWrapper::ImageType                     WsrcImageType;

  typedef itk::JoinCopyFilter<JsrcImageType, JdstImageType, JdstImageType> JoinCopyFilterType;
  typedef JoinCopyFilterType::Pointer JoinCopyFilterPointer;

  /** Initialize to a ROI from another image data object */
  void InitializeToROI(GenericImageData *source,
                       const SNAPSegmentationROISettings &roi,
                       itk::Command *progressCommand);

  /** Copy nickname, settings, and other such junk from IRIS to SNAP during
   * initialization */
  void CopyLayerMetadata(ImageWrapperBase *target, ImageWrapperBase *source);

  /**
    Unload all images in the JOIN image data, releasing memory and returning
    this object to initial state.
    */
  void UnloadAll();

  void InitializeJsrc(JsrcImageType *newJsrcImage= NULL);
  JsrcImageWrapper* GetJsrc();
  void SetJsrc(JsrcImageType *newJsrcImage);
  bool IsJsrcLoaded();
  
  void InitializeJdst();
  JdstImageWrapper* GetJdst();
  bool IsJdstLoaded();
  void SetJdstSticky(bool sticky);

  void InitializeWsrc();
  WsrcImageWrapper* GetWsrc();
  void SetWsrc(WsrcImageType *newWsrcImage);
  bool IsWsrcLoaded();
  void SetWsrcSticky(bool sticky);
  
  void InitializeJoinCF();
  JoinCopyFilterType::Pointer GetJoinCF();
  
protected:

  JOINImageData();
  ~JOINImageData();

  // Join source image
  SmartPtr<JsrcImageWrapper> m_JsrcWrapper;

  // Join destination image
  SmartPtr<JdstImageWrapper> m_JdstWrapper;

  // GWS source image
  SmartPtr<WsrcImageWrapper> m_WsrcWrapper;

  JoinCopyFilterPointer m_JoinCF;

};







#endif
