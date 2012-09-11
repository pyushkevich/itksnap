/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ReorientImageUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2009/10/30 16:48:22 $
  Version:   $Revision: 1.5 $
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

#include "SNAPCommonUI.h"
#include "ReorientImageUILogic.h"
#include "UserInterfaceLogic.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "GenericImageData.h"
#include "itkImage.h"
#include <cassert>
#include <string>
#include <iomanip>
#include <sstream>

std::string
PrintMatrixFormatted(
  vnl_matrix<double> mat, 
  size_t width, 
  size_t precision, 
  const char *prefix = "")
{
  // Create output stream
  std::ostringstream sout;

  // Print each row and column of the matrix
  for(size_t i = 0; i < mat.rows(); i++)
    {
    sout << prefix;
    for(size_t j = 0; j < mat.columns(); j++)
      {
      sout 
        << std::setw(width) 
        << std::setprecision(precision) 
        << mat(i,j);
      if(j == mat.columns() - 1)
        sout << std::endl;
      else 
        sout << " ";
      }
    }

  // Return result
  return sout.str();
}

const char ReorientImageUILogic::m_RAICodes[3][2] = {
  {'R', 'L'},
  {'A', 'P'},
  {'I', 'S'}};

const char *ReorientImageUILogic::m_AxisLabels[3][2] = {
  {"Right to Left", "Left to Right"},
  {"Anterior to Posterior", "Posterior to Anterior"},
  {"Inferior to Superior", "Superior to Inferior"}};

ReorientImageUILogic
::ReorientImageUILogic()
{

}

void
ReorientImageUILogic
::ShowDialog()
{
  assert(m_ParentUI);

  // Get the current image data  
  m_ImageData = m_ParentUI->GetDriver()->GetCurrentImageData();

  // Get the direction matrix
  vnl_matrix_fixed<double, 3, 3> dir = 
    m_ImageData->GetMain()->GetImageBase()->GetDirection().GetVnlMatrix();

  // Compute the RAI code from the direction matrix
  char rai[4]; rai[3] = 0;
  bool oblique = false;

  for(size_t i = 0; i < 3; i++)
    {
    // Get the direction cosine for voxel direction i
    Vector3d dcos = dir.get_column(i);
    double dabsmax = dcos.inf_norm();

    for(size_t j = 0; j < 3; j++)
      {
      double dabs = fabs(dcos[j]);
      size_t dsgn = dcos[j] > 0 ? 0 : 1;

      if(dabs == 1.0)
        {
        rai[i] = m_RAICodes[j][dsgn];
        m_OutCurrentAxisDirection[i]->value(m_AxisLabels[j][dsgn]);
        }
      else if(dabs == dabsmax)
        {
        oblique = true;
        rai[i] = m_RAICodes[j][dsgn];
        m_OutCurrentAxisDirection[i]->value("Oblique");
        }
      }
    }
      
  // If RAI code is valid, set all the other components
  if(oblique)
    {
    m_OutCurrentRAI->value("OBLIQUE");
    std::ostringstream sout;
    sout << "Closest to " << rai;
    m_OutCurrentRAIClosest->value(sout.str().c_str());
    m_InDesiredRAI->value(rai);
    }
  else
    {
    m_OutCurrentRAI->value(rai);
    m_OutCurrentRAIClosest->value("");
    m_InDesiredRAI->value(rai);
    }

  // Print the matrix
  string smat_current = 
    PrintMatrixFormatted(m_ImageData->GetMain()->GetNiftiSform(), 9, 3);
  m_OutCurrentSForm->value(smat_current.c_str());

  // Update the 'current' graphic
  this->UpdateOrientationGraphic(rai, oblique, m_GrpCurrentDoll, m_GrpCurrentDollAxis);

  // Update the other fields in the desired column
  this->UpdateDesiredDerivedFields();

  // Show the window
  m_WinReorient->show();
  m_ParentUI->CenterChildWindowInMainWindow(m_WinReorient);
}

void
ReorientImageUILogic
::UpdateDesiredDerivedFields()
{
  // An identity matrix, for pulling out rows
  vnl_matrix_fixed<double, 3, 3> eye;
  eye.set_identity();

  // Apply the RAI code
  const char *rai = m_InDesiredRAI->value();
  for(size_t i = 0; i < 3; i++)
    {
    for(size_t j = 0; j < 3; j++)
      {
      for(size_t k = 0; k < 2; k++)
        {
        if(toupper(rai[i]) == m_RAICodes[j][k])
          {
          m_OutDesiredAxisDirection[i]->value(m_AxisLabels[j][k]);
          m_DesiredDirection.set_column(i, (k==0 ? 1.0 : -1.0) * eye.get_row(j));
          }
        }
      }
    }

  // Compute the new NIFTI matrix given these directions
  vnl_matrix_fixed<double, 4, 4> m_sform = 
    GreyImageWrapper::ConstructNiftiSform(
      m_DesiredDirection,
      m_ImageData->GetMain()->GetImageBase()->GetOrigin().GetVnlVector(),
      m_ImageData->GetMain()->GetImageBase()->GetSpacing().GetVnlVector());

  // Print the matrix
  string smat_desired = 
    PrintMatrixFormatted(m_sform, 9, 3);
  m_OutDesiredSForm->value(smat_desired.c_str());  

  // Update the graphic
  this->UpdateOrientationGraphic(rai, false, m_GrpDesiredDoll, m_GrpDesiredDollAxis);
}

void
ReorientImageUILogic
::UpdateOrientationGraphic(
  const char *rai, bool oblique, Fl_Wizard *grpDoll, Fl_Wizard *grpDollAxis[])
{
  // Handle oblique/invalid cases (show question mark)
  if(oblique || !ImageCoordinateGeometry::IsRAICodeValid(rai))
    {
    grpDoll->value(grpDoll->child(6));
    return;
    }

  // Initialize the mappings for the doll display
  size_t xCodeToGraphicMap[3][3][3];
  xCodeToGraphicMap[0][1][2] = 0; // RAI
  xCodeToGraphicMap[0][2][1] = 1; // RIA
  xCodeToGraphicMap[1][0][2] = 2; // ARI
  xCodeToGraphicMap[1][2][0] = 3; // AIR
  xCodeToGraphicMap[2][0][1] = 4; // IRA
  xCodeToGraphicMap[2][1][0] = 5; // IAR

  // Convert RAI to a numeric mapping
  Vector3i rn = ImageCoordinateGeometry::ConvertRAIToCoordinateMapping(rai);
  Vector3i ra = rn.apply(abs);
  
  // Get the appropriate doll
  size_t idoll = xCodeToGraphicMap[ra[0]-1][ra[1]-1][ra[2]-1];

  // Select the doll
  grpDoll->value(grpDoll->child(idoll));

  // Geez, this is getting complicated!
  int xDollAxisFlips[6][3] = {
    { 0, 0, 0 },
    { 1, 1, 1 },
    { 1, 0, 0 }, 
    { 1, 1, 0 },
    { 0, 1, 1 },
    { 0, 0, 1 }}; 

  // Flip the arrows too
  for(size_t i = 0; i < 3; i++)
    {
    int axdir = (rn[i] > 0) 
      ? xDollAxisFlips[idoll][i] 
      : 1 - xDollAxisFlips[idoll][i];
    grpDollAxis[i]->value(grpDollAxis[i]->child(axdir));
    }
}

void
ReorientImageUILogic
::Register(UserInterfaceLogic *parent_ui)
{ 
  m_ParentUI = parent_ui; 
}

void 
ReorientImageUILogic
::OnDesiredRAIUpdate()
{
  // If the code is valid, update the UI  
  if(ImageCoordinateGeometry::IsRAICodeValid(m_InDesiredRAI->value()))
    {
    m_OutDesiredRAIError->value("");
    this->UpdateDesiredDerivedFields();
    }
  else
    {
    m_OutDesiredRAIError->value("Invalid RAI Code");
    m_OutDesiredRAIError->show();
    m_OutDesiredAxisDirection[0]->value("");
    m_OutDesiredAxisDirection[1]->value("");
    m_OutDesiredAxisDirection[2]->value("");
    m_OutDesiredSForm->value("");
    this->UpdateOrientationGraphic("", false, m_GrpDesiredDoll, m_GrpDesiredDollAxis);
    }
}

void 
ReorientImageUILogic
::OnOkAction()
{
  this->OnApplyAction();
  this->OnCloseAction();
}

void 
ReorientImageUILogic
::OnApplyAction()
{
  // Reorient the image
  m_ParentUI->GetDriver()->ReorientImage(m_DesiredDirection);

  // GUI has to refresh
  m_ParentUI->OnImageGeometryUpdate();
}

void 
ReorientImageUILogic
::OnCloseAction()
{
  m_WinReorient->hide();
}

bool
ReorientImageUILogic
::Shown()
{
  return m_WinReorient->shown();
}

