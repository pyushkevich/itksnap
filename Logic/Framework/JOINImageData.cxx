/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: SNAPImageData.cxx,v $
  Language:  C++
  Date:      $Date: 2011/04/18 17:35:30 $
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

#include "IRISApplication.h"
#include "JOINImageData.h"

JOINImageData
::JOINImageData(){
    }


JOINImageData
::~JOINImageData(){
    }


/* =============================
   Join source Image
   ============================= */

void 
JOINImageData
::InitializeJsrc(){
    // The Grey image wrapper should be present
    assert(m_MainImageWrapper->IsInitialized());

    // Intialize Jsrc based on the current grey image
    if(m_JsrcWrapper.IsNull())
	{
	m_JsrcWrapper = JsrcImageWrapper::New();
	m_JsrcWrapper->SetDefaultNickname("Join Source Image");
        PushBackImageWrapper(JOIN_ROLE, m_JsrcWrapper.GetPointer());
	}

    m_JsrcWrapper->InitializeToWrapper(m_MainImageWrapper, (JSRType) 0);
    m_JsrcWrapper->SetSticky(true); //overlay, ie no separate tile
    m_JsrcWrapper->SetAlpha(0.5);

    InvokeEvent(LayerChangeEvent());
    }

JsrcImageWrapper* 
JOINImageData
::GetJsrc(){
    // Make sure it exists
    assert(IsJsrcLoaded());
    return m_JsrcWrapper;
    }

void
JOINImageData
::SetJsrc(JsrcImageType *newJsrcImage){
    ////from ./Logic/Framework/GenericImageData.cxx:244:::SetSegmentationImage
    // Check that the image matches the size of the grey image
    assert(m_MainImageWrapper->IsInitialized());

    assert(m_MainImageWrapper->GetBufferedRegion() ==
	newJsrcImage->GetBufferedRegion());

    // Pass the image to the wrapper
    m_JsrcWrapper->SetImage(newJsrcImage);
    m_JsrcWrapper->GetImage()->Modified();// This makes sure that the IsDrawable() of the wrapper returns true, essential for showing up in the the SliceView (l.284 GenericSliceRenderer.cxx)

    // Sync up spacing between the main and label image
    newJsrcImage->SetSpacing(m_MainImageWrapper->GetImageBase()->GetSpacing());
    newJsrcImage->SetOrigin(m_MainImageWrapper->GetImageBase()->GetOrigin());
    }

bool 
JOINImageData
::IsJsrcLoaded(){
    return m_JsrcWrapper && m_JsrcWrapper->IsInitialized();
    }


/* =============================
   Join destination Image
   ============================= */

void
JOINImageData
::InitializeJdst(){
    assert(IsJsrcLoaded());

    // If a initialization wrapper does not exist, create it
    if(!m_JdstWrapper)
	{
	m_JdstWrapper = JdstImageWrapper::New();
	m_JdstWrapper->SetDefaultNickname("Join Destination Image");
	PushBackImageWrapper(JOIN_ROLE, m_JdstWrapper.GetPointer());
	}

    m_JdstWrapper->GetDisplayMapping()->SetLabelColorTable(m_Parent->GetColorLabelTable());
    m_JdstWrapper->InitializeToWrapper(m_MainImageWrapper, (LabelType) 0);
    m_JdstWrapper->SetSticky(true);//sticky is set with SetJdstSticky
    m_JdstWrapper->SetAlpha(0.5);

    InvokeEvent(LayerChangeEvent());
    }

JdstImageWrapper* 
JOINImageData
::GetJdst(){
    assert(IsJdstLoaded());
    return m_JdstWrapper;
    }

bool 
JOINImageData
::IsJdstLoaded() 
{
  return (m_JdstWrapper && m_JdstWrapper->IsInitialized());
}

void 
JOINImageData
::SetJdstSticky(bool sticky){
    m_JdstWrapper->SetSticky(sticky);
    }

/* =============================
   GWS source Image
   ============================= */

void 
JOINImageData
::InitializeWsrc(){
    // The Grey image wrapper should be present
    assert(m_MainImageWrapper->IsInitialized());

    // Intialize Wsrc based on the current grey image
    if(m_WsrcWrapper.IsNull())
	{
	m_WsrcWrapper = WsrcImageWrapper::New();
	m_WsrcWrapper->SetDefaultNickname("GWS Source Image");
	PushBackImageWrapper(JOIN_ROLE, m_WsrcWrapper.GetPointer());
	}

    m_WsrcWrapper->InitializeToWrapper(m_MainImageWrapper, (WSRType) 0);
    m_WsrcWrapper->SetSticky(true); //sticky is set with SetWsrcSticky
    m_WsrcWrapper->SetAlpha(0.5);

    InvokeEvent(LayerChangeEvent());
    }

WsrcImageWrapper* 
JOINImageData
::GetWsrc(){
    // Make sure it exists
    assert(IsWsrcLoaded());
    return m_WsrcWrapper;
    }

void
JOINImageData
::SetWsrc(WsrcImageType *newWsrcImage){
    ////from ./Logic/Framework/GenericImageData.cxx:244:::SetSegmentationImage
    // Check that the image matches the size of the grey image
    assert(m_MainImageWrapper->IsInitialized());

    assert(m_MainImageWrapper->GetBufferedRegion() ==
	newWsrcImage->GetBufferedRegion());

    // Pass the image to the wrapper
    m_WsrcWrapper->SetImage(newWsrcImage);
    m_WsrcWrapper->GetImage()->Modified();// This makes sure that the IsDrawable() of the wrapper returns true, essential for showing up in the the SliceView (l.284 GenericSliceRenderer.cxx)

    // Sync up spacing between the main and label image
    newWsrcImage->SetSpacing(m_MainImageWrapper->GetImageBase()->GetSpacing());
    newWsrcImage->SetOrigin(m_MainImageWrapper->GetImageBase()->GetOrigin());
    }

bool 
JOINImageData
::IsWsrcLoaded(){
    return m_WsrcWrapper && m_WsrcWrapper->IsInitialized();
    }

void 
JOINImageData
::SetWsrcSticky(bool sticky){
    m_WsrcWrapper->SetSticky(sticky);
    }

/**********************************/

void
JOINImageData
::InitializeToROI(GenericImageData *source,
    const SNAPSegmentationROISettings &roi,
    itk::Command *progressCommand){
    // Get the source grey wrapper
    AnatomicImageWrapper *srcWrapper = source->GetMain();

    // Get the roi chunk from the grey image
    SmartPtr<AnatomicImageType> imgNew =
	srcWrapper->DeepCopyRegion(roi, progressCommand); //only roi of grey
    //source->GetMain()->GetImage(); //full grey, only roi GWS processed

    // Get the size of the region
    Vector3ui size = imgNew->GetLargestPossibleRegion().GetSize();

    // Compute an image coordinate geometry for the region of interest
    std::string rai[] = {
	this->m_Parent->GetDisplayToAnatomyRAI(0),
	this->m_Parent->GetDisplayToAnatomyRAI(1),
	this->m_Parent->GetDisplayToAnatomyRAI(2) };
    ImageCoordinateGeometry icg(
        source->GetImageGeometry().GetImageDirectionCosineMatrix(),
        rai, size);

    // Assign the new wrapper to the target
    this->SetMainImage(imgNew, icg, srcWrapper->GetNativeMapping());

    // Copy metadata
    this->CopyLayerMetadata(this->GetMain(), source->GetMain());

    // Repeat all of this for the overlays
    for(LayerIterator lit = source->GetLayers(OVERLAY_ROLE);
	!lit.IsAtEnd(); ++lit)
	{
	// Cast the layer to anatomic image wrapper type
	AnatomicImageWrapper *ovlWrapper =
	    dynamic_cast<AnatomicImageWrapper *>(lit.GetLayer());

	assert(ovlWrapper);

	// Create a copy of the layer for the requested ROI
	SmartPtr<AnatomicImageType> ovlNew = ovlWrapper->DeepCopyRegion(roi, progressCommand);

	// Add the overlay
	this->AddOverlay(ovlNew, ovlWrapper->GetNativeMapping());

	// Copy metadata
	this->CopyLayerMetadata(this->GetLastOverlay(), ovlWrapper);
	}
    }

void JOINImageData::CopyLayerMetadata(
    ImageWrapperBase *target, ImageWrapperBase *source){
    // Nickname
    target->SetDefaultNickname(source->GetNickname());

    // This is a little bit of overhead, but not enough to be a big deal:
    // we just save the display mapping to a Registry and then restore it
    // in the target wrapper.
    Registry folder;
    source->GetDisplayMapping()->Save(folder);
    target->GetDisplayMapping()->Restore(folder);

    // Threshold settings. These should be copied for each scalar component
    if(source->IsScalar())
	{
	target->SetUserData("ThresholdSettings", source->GetUserData("ThresholdSettings"));
	}
    else
	{
	// Copy threshold settings for all the scalar components
	VectorImageWrapperBase *v_source = dynamic_cast<VectorImageWrapperBase *>(source);
	VectorImageWrapperBase *v_target = dynamic_cast<VectorImageWrapperBase *>(target);

	for(ScalarRepresentationIterator it(v_source); !it.IsAtEnd(); ++it)
	    {
	    ImageWrapperBase *c_source = v_source->GetScalarRepresentation(it);
	    ImageWrapperBase *c_target = v_target->GetScalarRepresentation(it);
	    c_target->SetUserData("ThresholdSettings", c_source->GetUserData("ThresholdSettings"));
	    }
	}

    // TODO: alpha, stickiness?
    }

void JOINImageData::UnloadAll(){
    // Unload all the data
    this->UnloadOverlays();
    this->UnloadMainImage(); //do not unload if Main just points to IRIS-Main!!!

    // We need to unload all the JOIN layers
    while(this->m_Wrappers[JOIN_ROLE].size())
	PopBackImageWrapper(JOIN_ROLE);
    m_JsrcWrapper = NULL;
    m_JdstWrapper = NULL;
    m_WsrcWrapper = NULL;

    InvokeEvent(LayerChangeEvent());
    }

