#include <string>

#include <QMenu>
#include <qstring.h>
#include <QFileDialog>
#include <QWheelEvent>


#include "ReorientGUI.h"
#include "Reorient.h"
#include "ScanningROI.h"

using namespace std;


ReorientGUI::ReorientGUI()
{	

  setupUi(this);
  
  m_pQVTKWidget->SetRenderWindow(m_Reorient.GetRenderWindow());
  m_Reorient.setInteractor(m_pQVTKWidget->GetInteractor());
  
  vtkSmartPointer < vtkMatrix4x4 > pMatrix = vtkSmartPointer < vtkMatrix4x4 >::New();
  m_Reorient.GetDirections(pMatrix);

  int nI, nJ;
  for(nI = 0; nI < 4; nI++)
  {
	for(nJ = 0; nJ < 4; nJ++)
    {
	  double dbEquals = (*pMatrix)[nI][nJ];

	  QTableWidgetItem* pItem= new QTableWidgetItem();
	  pItem->setText(QString::number(dbEquals));
	  m_pTableWidget->setItem(nI,nJ,pItem); 
    }
  }

  connect(m_pTableWidget,SIGNAL( cellChanged (int, int) ), this, SLOT( slotReorient(int, int) ));

  connect(m_pDoubleSpinBoxRotPhi,SIGNAL( valueChanged(double) ), this, SLOT( slotPhiThetaPsi(double) ));
  connect(m_pDoubleSpinBoxRotTheta,SIGNAL( valueChanged(double) ), this, SLOT( slotPhiThetaPsi(double) ));
  connect(m_pDoubleSpinBoxRotPsi,SIGNAL( valueChanged(double) ), this, SLOT( slotPhiThetaPsi(double) ));

  connect(m_pRadioButtonInterpretNegativeOrientation3x3,SIGNAL( toggled(bool) ), this, SLOT( slotSelectNegativeOrientation(bool) ));
  
}

ReorientGUI::~ReorientGUI()
{
}

vtkSmartPointer < vtkMatrix4x4 > ReorientGUI::getMtrx4x4GUI()
{
  vtkSmartPointer < vtkMatrix4x4 > pMatrix4x4 = vtkSmartPointer < vtkMatrix4x4 >::New();
  int nI, nJ;
  for(nI = 0; nI < 4; nI++)
  {
	for(nJ = 0; nJ < 4; nJ++)
    {
	  QTableWidgetItem* pItem = m_pTableWidget->item(nI, nJ);
	  (*pMatrix4x4)[nI][nJ] = pItem->text().toDouble(); 
    }
  }
  return(pMatrix4x4);
}

void ReorientGUI::slotReorient(int anDummyRow, int anDummyCol)
{
  vtkSmartPointer < vtkMatrix4x4 > pMatrix4x4 = getMtrx4x4GUI();
  m_Reorient.Update(pMatrix4x4);
}

void ReorientGUI::slotPhiThetaPsi(double adbValue)
{
  disconnect(m_pTableWidget,SIGNAL( cellChanged (int, int) ), this, SLOT( slotReorient(int, int) ));

  double dbPhi = m_pDoubleSpinBoxRotPhi->value();
  double dbTheta = m_pDoubleSpinBoxRotTheta->value();
  double dbPsi = m_pDoubleSpinBoxRotPsi->value();

  double cosPhi = cos(dbPhi);
  double sinPhi = sin(dbPhi);
  double cosTheta = cos(dbTheta);
  double sinTheta = sin(dbTheta);
  double cosPsi = cos(dbPsi);
  double sinPsi = sin(dbPsi);

  //Formulas taken from http://en.wikipedia.org/wiki/Rotation_formalisms_in_three_dimensions
  vtkSmartPointer < vtkMatrix4x4 > pMatrix4x4 = vtkSmartPointer < vtkMatrix4x4 >::New();
  (*pMatrix4x4)[0][0] = cosTheta  * cosPsi;
  (*pMatrix4x4)[0][1] = cosPhi    * sinPsi + sinPhi * sinTheta * cosPsi;
  (*pMatrix4x4)[0][2] = sinPhi    * sinPsi - cosPhi * sinTheta * cosPsi;

  (*pMatrix4x4)[1][0] = -cosTheta * sinPsi;
  (*pMatrix4x4)[1][1] =  cosPhi   * cosPsi - sinPhi * sinTheta * sinPsi;
  (*pMatrix4x4)[1][2] =  sinPhi   * cosPsi + cosPhi * sinTheta * sinPsi;

  (*pMatrix4x4)[2][0] = sinTheta;
  (*pMatrix4x4)[2][1] = -sinPhi   * cosTheta;
  (*pMatrix4x4)[2][2] = cosPhi    * cosTheta;

  int nI, nJ;
  for(nI = 0; nI < 4; nI++)
  {
	for(nJ = 0; nJ < 4; nJ++)
    {
	  QTableWidgetItem* pItem = m_pTableWidget->item(nI, nJ);
	  pItem->setText(QString::number((*pMatrix4x4)[nI][nJ]));
    }
  }
  pMatrix4x4->Transpose();
  
  bool bInterpretNegativeOrientation3x3 = m_pRadioButtonInterpretNegativeOrientation3x3->isChecked();
  if(bInterpretNegativeOrientation3x3 == true)
    {
	ScanningROI::changeOrientation3x3(pMatrix4x4);
    }
  m_Reorient.Update(pMatrix4x4);

  connect(m_pTableWidget,SIGNAL( cellChanged (int, int) ), this, SLOT( slotReorient(int, int) ));
}

void ReorientGUI::slotSelectNegativeOrientation(bool abInterpretNegativeOrientation3x3)
{
  vtkSmartPointer < vtkMatrix4x4 > pMatrix4x4 =
	//vtkSmartPointer < vtkMatrix4x4 >::New();
  getMtrx4x4GUI();
  pMatrix4x4->Transpose();
  if(abInterpretNegativeOrientation3x3 == true)
    {
	ScanningROI::changeOrientation3x3(pMatrix4x4);
    }
  m_Reorient.Update(pMatrix4x4);
}
