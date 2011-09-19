#include "CursorInspector.h"
#include "ui_CursorInspector.h"
#include <VoxelIntensityQTableModel.h>
#include <QSpinBox>
#include <GenericImageData.h>
#include <IRISApplication.h>
#include <GlobalUIModel.h>


CursorInspector::CursorInspector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CursorInspector)
{
  ui->setupUi(this);
  m_TableModel = new VoxelIntensityQTableModel(ui->tableView);
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

  // Listen to cursor changes
  AddListener(m_Model->GetDriver(), CursorUpdateEvent(),
              this, &CursorInspector::UpdateUIFromModel);
}

void CursorInspector::UpdateUIFromModel()
{
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
}
