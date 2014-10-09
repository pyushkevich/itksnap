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

    m_JsrcWrapper->GetDisplayMapping()->SetLabelColorTable(m_Parent->GetColorLabelTable());
    m_JsrcWrapper->InitializeToWrapper(m_MainImageWrapper, (JSRType) 0);
    m_JsrcWrapper->SetSticky(true); //overlay, ie no separate tile
    m_JsrcWrapper->SetAlpha(0.3);

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

    // Sync up spacing between the main and label image
    newJsrcImage->SetSpacing(m_MainImageWrapper->GetImageBase()->GetSpacing());
    newJsrcImage->SetOrigin(m_MainImageWrapper->GetImageBase()->GetOrigin());
    }

bool 
JOINImageData
::IsJsrcLoaded(){
    return m_JsrcWrapper && m_JsrcWrapper->IsInitialized();
    }

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
    m_JdstWrapper->SetSticky(false);
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

    InvokeEvent(LayerChangeEvent());
    }

