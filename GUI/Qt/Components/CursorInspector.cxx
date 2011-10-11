#include "CursorInspector.h"
#include "ui_CursorInspector.h"
#include <VoxelIntensityQTableModel.h>
#include <QSpinBox>
#include <GenericImageData.h>
#include <IRISApplication.h>
#include <GlobalUIModel.h>
#include <QtWidgetActivator.h>

CursorInspector::CursorInspector(QWidget *parent) :
    SNAPComponent(parent),
    ui(new Ui::CursorInspector)
{
  ui->setupUi(this);
  m_TableModel = new VoxelIntensityQTableModel(ui->tableView);

  // Connect widgets (delayed)
  QSpinBox *sp[] = {ui->inCursorX, ui->inCursorY, ui->inCursorZ};
  for(unsigned int i = 0; i < 3; i++)
    {
    connect(sp[i], SIGNAL(valueChanged(int)), SLOT(onCursorEdit()),
            Qt::QueuedConnection);
    sp[i]->setKeyboardTracking(false);
    }
}

CursorInspector::~CursorInspector()
{
  delete ui;
}

void CursorInspector::SetModel(GlobalUIModel *model)
{
  m_Model = model;
  m_TableModel->SetParentModel(model);
  ui->tableView->setModel(m_TableModel);
  ui->tableView->setAlternatingRowColors(true);
  ui->tableView->setFixedWidth(160);
  ui->tableView->setFixedHeight(120);

  // Update UI from model
  UpdateUIFromModel();

  // Activators
  activateOnFlag(this, m_Model, UIF_BASEIMG_LOADED);

  // Listen to cursor changes
  connectITK(m_Model, CursorUpdateEvent());
}

void CursorInspector::onModelUpdate(const EventBucket &)
{
  UpdateUIFromModel();
}

void CursorInspector::UpdateUIFromModel()
{
  m_Model->Update();

  // Get current data
  IRISApplication *app = m_Model->GetDriver();
  GenericImageData *gid = app->GetCurrentImageData();

  if(gid->IsMainLoaded())
    {
    // Update the cursor display
    QSpinBox *sp[] = {ui->inCursorX, ui->inCursorY, ui->inCursorZ};
    for (unsigned int i = 0; i < 3; i++)
      {
      sp[i]->setMinimum(0);
      sp[i]->setMaximum(gid->GetMain()->GetSize()[i]-1);
      sp[i]->setValue(app->GetCursorPosition()[i]);
      sp[i]->setSingleStep(1);
      }

    // Update the label display
    if(gid->IsSegmentationLoaded())
      {
      LabelType label = gid->GetSegmentation()->GetVoxel(app->GetCursorPosition());
      ui->outLabelId->setValue(label);
      ui->outLabelText->setText(app->GetColorLabelTable()->GetColorLabel(label).GetLabel());
      }
    }

  ui->tableView->update();
}

void CursorInspector::onCursorEdit()
{
  // Set the cursor from the current edits
  m_Model->GetDriver()->SetCursorPosition(Vector3ui(ui->inCursorX->value(),
                                                    ui->inCursorY->value(),
                                                    ui->inCursorZ->value()));
}
