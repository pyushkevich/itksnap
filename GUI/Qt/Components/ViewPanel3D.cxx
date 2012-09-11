#include "ViewPanel3D.h"
#include "ui_ViewPanel3D.h"
#include "GlobalUIModel.h"
#include "Generic3DModel.h"
#include "itkCommand.h"
#include "IRISException.h"

ViewPanel3D::ViewPanel3D(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::ViewPanel3D)
{
  ui->setupUi(this);
}

ViewPanel3D::~ViewPanel3D()
{
  delete ui;
}

void ViewPanel3D::OnRenderProgress()
{
  // TODO: fix this, add progress bar
  std::cout << "." << std::flush;
}

void ViewPanel3D::on_btnUpdateMesh_clicked()
{
  // Do something about a progress bar
  typedef itk::SimpleMemberCommand<ViewPanel3D> CommandType;
  SmartPtr<CommandType> cmd = CommandType::New();
  cmd->SetCallbackFunction(this, &ViewPanel3D::OnRenderProgress);

  try {
    // Tell the model to update itself
    m_Model->UpdateSegmentationMesh(cmd);
  } catch(IRISException & IRISexc) {
      cerr << IRISexc.what() << endl;
  }

  // Delete this later - should be automatic!
  ui->view3d->repaint();
}

void ViewPanel3D::Initialize(GlobalUIModel *globalUI)
{
  // Save the model
  m_GlobalUI = globalUI;
  m_Model = globalUI->GetModel3D();
  ui->view3d->SetModel(m_Model);
}
