/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

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

=========================================================================*/

#include "ZoomInspector.h"
#include "ui_ZoomInspector.h"

#include "GlobalUIModel.h"
#include "SliceWindowCoordinator.h"
#include "QtWidgetActivator.h"
#include "QtDoubleSpinBoxCoupling.h"
#include "QtCheckBoxCoupling.h"
#include <SNAPQtCommon.h>
#include <vnl/vnl_math.h>

ZoomInspector::ZoomInspector(QWidget *parent) :
    SNAPComponent(parent),
    ui(new Ui::ZoomInspector)
{
  ui->setupUi(this);

}

ZoomInspector::~ZoomInspector()
{
  delete ui;
}


void ZoomInspector::SetModel(GlobalUIModel *model)
{
  // Set the model
  m_Model = model;

  // Connect buttons to global actions
  ui->btnResetViews->setAction("actionZoomToFitInAllViews");
  ui->btnCenterViews->setAction("actionCenter_on_Cursor");

  // Conditional activation of widgets
  activateOnFlag(ui->inZoom, model, UIF_LINKED_ZOOM);
  activateOnFlag(ui->btnZoom1, model, UIF_LINKED_ZOOM);
  activateOnFlag(ui->btnZoom2, model, UIF_LINKED_ZOOM);
  activateOnFlag(ui->btnZoom4, model, UIF_LINKED_ZOOM);

  activateOnFlag(this, model, UIF_BASEIMG_LOADED);

  // Couple the linked zoom checkbox
  makeCoupling(ui->chkLinkedZoom,
               model->GetSliceCoordinator()->GetLinkedZoomModel());

  // Couple zoom widget to the linked zoom level
  makeCoupling(ui->inZoom,
               model->GetSliceCoordinator()->GetCommonZoomFactorModel());
}

void ZoomInspector::on_chkLinkedZoom_stateChanged(int state)
{
  m_Model->GetSliceCoordinator()->SetLinkedZoom(state == Qt::Checked);
}

void ZoomInspector::on_btnZoom1_pressed()
{
  m_Model->GetSliceCoordinator()->SetZoomPercentageInAllWindows(1);
}

void ZoomInspector::on_btnZoom2_pressed()
{
  m_Model->GetSliceCoordinator()->SetZoomPercentageInAllWindows(2);
}

void ZoomInspector::on_btnZoom4_pressed()
{
  m_Model->GetSliceCoordinator()->SetZoomPercentageInAllWindows(4);
}
