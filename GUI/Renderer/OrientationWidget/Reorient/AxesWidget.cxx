#include "vtkObjectFactory.h"
#include "vtkAxesActor.h"
#include "vtkSmartPointer.h"
#include "vtkCaptionActor2D.h"
#include "vtkTextProperty.h"
#include "vtkProperty.h"

#include "AxesWidget.h"

const double AxesWidget::m_arrdbColRed[3] = {1.0, 0.0, 0.0};
const double AxesWidget::m_arrdbColGreen[3] = {0.0, 1.0, 0.0};
const double AxesWidget::m_arrdbColBlue[3] = {0.0, 0.0, 1.0};

AxesWidget::AxesWidget()
{
 
  m_pvtkAxesActor = vtkSmartPointer < vtkAxesActor > ::New();

  m_pvtkAxesActor->SetShaftTypeToCylinder();
  m_pvtkAxesActor->SetXAxisLabelText( "x" );
  m_pvtkAxesActor->SetYAxisLabelText( "y" );
  m_pvtkAxesActor->SetZAxisLabelText( "z" );

  m_pvtkAxesActor->SetTotalLength( 1.5, 1.5, 1.5 );
  m_pvtkAxesActor->SetCylinderRadius( 0.500 * m_pvtkAxesActor->GetCylinderRadius() );
  m_pvtkAxesActor->SetConeRadius    ( 1.025 * m_pvtkAxesActor->GetConeRadius() );
  m_pvtkAxesActor->SetSphereRadius  ( 1.500 * m_pvtkAxesActor->GetSphereRadius() );

  m_pvtkAxesActor->SetConeResolution(100);
  m_pvtkAxesActor->SetCylinderResolution(100);

  vtkTextProperty* tprop = m_pvtkAxesActor->GetXAxisCaptionActor2D()->
    GetCaptionTextProperty();
  tprop->ItalicOn();
  tprop->ShadowOn();
  tprop->SetFontFamilyToTimes();

  m_pvtkAxesActor->GetYAxisCaptionActor2D()->GetCaptionTextProperty()->ShallowCopy( tprop );
  m_pvtkAxesActor->GetZAxisCaptionActor2D()->GetCaptionTextProperty()->ShallowCopy( tprop );

}

vtkStandardNewMacro(AxesWidget);

void AxesWidget::VisibilityOn()
{
  m_pvtkAxesActor->VisibilityOn();
}

void AxesWidget::VisibilityOff()
{
  m_pvtkAxesActor->VisibilityOff();
}

int AxesWidget::GetVisibility()
{
  return(m_pvtkAxesActor->GetVisibility());
}

void AxesWidget::SetVisibility(int Visibility) {
  m_pvtkAxesActor->SetVisibility(Visibility);

  m_pvtkAxesActor->GetXAxisCaptionActor2D()->SetVisibility(Visibility);
  m_pvtkAxesActor->GetYAxisCaptionActor2D()->SetVisibility(Visibility);
  m_pvtkAxesActor->GetZAxisCaptionActor2D()->SetVisibility(Visibility);
}

void AxesWidget::SetLengths(double adbLength)
{
  m_pvtkAxesActor->SetTotalLength
    //SetNormalizedShaftLength
    (adbLength, adbLength, adbLength);
}

vtkSmartPointer < vtkAxesActor > AxesWidget::GetAxesActor()
{
  return(m_pvtkAxesActor);
}

void AxesWidget::SetLabels(const char * apchX, const char * apchY, const char * apchZ)
{
  m_pvtkAxesActor->SetXAxisLabelText( apchX );
  m_pvtkAxesActor->SetYAxisLabelText( apchY );
  m_pvtkAxesActor->SetZAxisLabelText( apchZ );
}

void AxesWidget::SetColors(const double aarrdbX[3], const double aarrdbY[3], const double aarrdbZ[3])
{

  vtkSmartPointer < vtkAxesActor > pAxesActor = GetAxesActor();

  double arrdbX[3], arrdbY[3], arrdbZ[3];
  memcpy(arrdbX, aarrdbX, 3 * sizeof(double));
  memcpy(arrdbY, aarrdbY, 3 * sizeof(double));
  memcpy(arrdbZ, aarrdbZ, 3 * sizeof(double));

  pAxesActor->GetXAxisTipProperty()->SetColor(arrdbX);
  pAxesActor->GetYAxisTipProperty()->SetColor(arrdbY);
  pAxesActor->GetZAxisTipProperty()->SetColor(arrdbZ);

  pAxesActor->GetXAxisShaftProperty()->SetColor(arrdbX);
  pAxesActor->GetYAxisShaftProperty()->SetColor(arrdbY);
  pAxesActor->GetZAxisShaftProperty()->SetColor(arrdbZ);

}
