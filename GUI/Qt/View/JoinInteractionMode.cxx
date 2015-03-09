#include "JoinInteractionMode.h"
#include "SliceViewPanel.h"
#include "GlobalUIModel.h"

JoinInteractionMode::JoinInteractionMode(GenericSliceView *parent)
    : SliceWindowInteractionDelegateWidget(parent){
    m_Model = NULL;
    }

JoinInteractionMode::~JoinInteractionMode(){
    }

void
JoinInteractionMode
::SetModel(JoinModel *model){
    m_Model = model;
    SetParentModel(model->GetParent());
    }

void JoinInteractionMode::mousePressEvent(QMouseEvent *ev){

    if(m_Model->GetParent()->GetParentUI()->CheckState(UIF_JOIN_MODE)){
	bool isleft = (ev->button() == Qt::LeftButton);
	bool isright = (ev->button() == Qt::RightButton);

	Qt::KeyboardModifiers modifiers = ev->modifiers();
	bool ctrl= modifiers.testFlag( Qt::ControlModifier);

	if(isleft || isright)
	    {
	    if(m_Model->ProcessPushEvent(to_float(m_XSlice),isright,ctrl))//ProcessPushEvent currently always returns false
		ev->accept(); //eat event, i.e. no cursor chasing
	    }
	}
    }

void JoinInteractionMode::enterEvent(QEvent *){
    // TODO: this is hideous!
    SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
    panel->SetMouseMotionTracking(true);
    }

void JoinInteractionMode::leaveEvent(QEvent *){
    SliceViewPanel *panel = dynamic_cast<SliceViewPanel *>(m_ParentView->parent());
    panel->SetMouseMotionTracking(false);

    // This fixes a crash when you press quit in paintbrush mode
    if(panel->isVisible())
	m_Model->ProcessLeaveEvent();
    }
