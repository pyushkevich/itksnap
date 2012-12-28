#include "DisplayLayoutInspector.h"
#include "ui_DisplayLayoutInspector.h"
#include "DisplayLayoutModel.h"
#include <QtRadioButtonCoupling.h>

DisplayLayoutInspector::DisplayLayoutInspector(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::DisplayLayoutInspector)
{
  ui->setupUi(this);
}

DisplayLayoutInspector::~DisplayLayoutInspector()
{
  delete ui;
}

void DisplayLayoutInspector::SetModel(DisplayLayoutModel *model)
{
  m_Model = model;

  // Couple the view panel layout toolbar buttons to the model
  QObjectList rlist;
  rlist.append(ui->btnFourViews);
  rlist.append(ui->btnAxial);
  rlist.append(ui->btnCoronal);
  rlist.append(ui->btnSagittal);
  rlist.append(ui->btn3D);
  makeRadioGroupCoupling(ui->grpDisplayLayout,
                         m_Model->GetViewPanelLayoutModel(), rlist);

  // Couple the radio buttons for layer layout
  makeRadioGroupCoupling(ui->grpLayerLayout,
                         m_Model->GetSliceViewLayerLayoutModel());
}
