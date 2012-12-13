#include "vtkObjectFactory.h"
#include "vtkAxesActor.h"
#include "vtkSmartPointer.h"
#include "vtkCaptionActor2D.h"
#include "vtkTextProperty.h"

#include "AxesWidget.h"

AxesWidget::AxesWidget()
{
  //vtkProperty* property = cube->GetCubeProperty();
 
  m_pvtkAxesActor = vtkSmartPointer < vtkAxesActor > ::New();

  m_pvtkAxesActor->SetShaftTypeToCylinder();
  m_pvtkAxesActor->SetXAxisLabelText( "x" );
  m_pvtkAxesActor->SetYAxisLabelText( "y" );
  m_pvtkAxesActor->SetZAxisLabelText( "z" );

  m_pvtkAxesActor->SetTotalLength( 1.5, 1.5, 1.5 );
  m_pvtkAxesActor->SetCylinderRadius( 0.500 * m_pvtkAxesActor->GetCylinderRadius() );
  m_pvtkAxesActor->SetConeRadius    ( 1.025 * m_pvtkAxesActor->GetConeRadius() );
  m_pvtkAxesActor->SetSphereRadius  ( 1.500 * m_pvtkAxesActor->GetSphereRadius() );

  vtkTextProperty* tprop = m_pvtkAxesActor->GetXAxisCaptionActor2D()->
    GetCaptionTextProperty();
  tprop->ItalicOn();
  tprop->ShadowOn();
  tprop->SetFontFamilyToTimes();

  m_pvtkAxesActor->GetYAxisCaptionActor2D()->GetCaptionTextProperty()->ShallowCopy( tprop );
  m_pvtkAxesActor->GetZAxisCaptionActor2D()->GetCaptionTextProperty()->ShallowCopy( tprop );

}

//void AxesWidget::Delete() {
//	VTK_DELETE_NULL(m_pvtkAxesActor);
//}
//
//AxesWidget::~AxesWidget() {
//	Delete();
//}

vtkStandardNewMacro(AxesWidget);

void AxesWidget::VisibilityOn() {
	m_pvtkAxesActor->VisibilityOn();
}

void AxesWidget::VisibilityOff() {
	m_pvtkAxesActor->VisibilityOff();
}

int AxesWidget::GetVisibility() {
	return(m_pvtkAxesActor->GetVisibility());
}

void AxesWidget::SetVisibility(int anVisible) {
	m_pvtkAxesActor->SetVisibility(anVisible);
}

void AxesWidget::SetLengths(double adbLength) {
	m_pvtkAxesActor->SetTotalLength
		//SetNormalizedShaftLength
		(adbLength, adbLength, adbLength);
}

vtkSmartPointer < vtkAxesActor > AxesWidget::GetAxesActor() {
	return(m_pvtkAxesActor);
}

void AxesWidget::SetLabels(const char * apchX, const char * apchY, const char * apchZ) {
	m_pvtkAxesActor->SetXAxisLabelText( apchX );
	m_pvtkAxesActor->SetYAxisLabelText( apchY );
	m_pvtkAxesActor->SetZAxisLabelText( apchZ );
}