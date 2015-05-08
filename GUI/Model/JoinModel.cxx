#include "JoinModel.h"
#include "GlobalState.h"
#include "IRISApplication.h"
#include "JOINImageData.h"

JoinModel::JoinModel(){
    }

void JoinModel::SetParent(GenericSliceModel *parent){
    // Set up the parent
    m_Parent = parent;
    }

void JoinModel::ComputeMousePosition(const Vector3f &xSlice){
    // Only when an image is loaded
    if(!m_Parent->GetDriver()->IsMainImageLoaded())
	return;

    // Compute the new cross-hairs position in image space
    Vector3f xCross = m_Parent->MapSliceToImage(xSlice);

    // Round the cross-hairs position down to integer
    Vector3i xCrossInteger = to_int(xCross);

    // Make sure that the cross-hairs position is within bounds by clamping
    // it to image dimensions
    Vector3i xSize =
	to_int(m_Parent->GetDriver()->GetCurrentImageData()->GetVolumeExtents());

    Vector3ui newpos = to_unsigned_int(
	xCrossInteger.clamp(Vector3i(0),xSize - Vector3i(1)));

    if(newpos != m_MousePosition || m_MouseInside == false)
	{
	m_MousePosition = newpos;
	m_MouseInside = true;
	//InvokeEvent(PaintbrushMovedEvent());
	}
    }

bool
JoinModel
::ProcessPushEvent(const Vector3f &xSlice, bool reverse_mode, bool ctrl){
    // Compute the mouse position
    ComputeMousePosition(xSlice);

    if(ctrl){
	processPickLabel(reverse_mode);
	}
    else {
	// Check if the right button was pressed
	if(processCnJ(reverse_mode)){
	    //add undo point (todo: Undo has no effect so far)
	    m_Parent->GetDriver()->StoreUndoPoint("Click'n'Join");
	    //fire SegmentationChangeEvent for e.g. 3D view Update
	    m_Parent->GetDriver()->InvokeEvent(SegmentationChangeEvent());
	    }
	}

    ////do not eat event, i.e. do cursor chasing; only works for LMB not for RMB!!!
    return false;
    }

void JoinModel::ProcessLeaveEvent(){
    m_MouseInside = false;
    }

void
JoinModel::processPickLabel(bool reverse_mode){

    // Get the global objects
    IRISApplication *driver = m_Parent->GetDriver();
    GlobalState *gs = driver->GetGlobalState();

    // Get Join destination image
    JdstImageWrapper *jdst = driver->GetJOINImageData()->GetJdst();
    JdstImageWrapper::ImageType::ValueType JdstClickPV= jdst->GetImage()->GetPixel(to_itkIndex(m_MousePosition));

    if(!reverse_mode){
	// ColorLabelTable::ValidLabelConstIterator pos =
	//     driver->GetColorLabelTable()->GetValidLabels().find(JdstClickPV);
	// gs->SetDrawingColorLabel(pos);
	gs->SetDrawingColorLabel(JdstClickPV);
	}
    else{
	DrawOverFilter dof = gs->GetDrawOverFilter();
	dof.CoverageMode = PAINT_OVER_ONE;
	dof.DrawOverLabel = JdstClickPV;
	gs->SetDrawOverFilter(dof);
	}
    }

bool
JoinModel::processCnJ(bool reverse_mode){
    // Get the global objects
    IRISApplication *driver = m_Parent->GetDriver();

    // Flag to see if anything was changed
    bool flagUpdate= driver->ExecuteCnJCopy(to_itkIndex(m_MousePosition));

    return flagUpdate;
    }

