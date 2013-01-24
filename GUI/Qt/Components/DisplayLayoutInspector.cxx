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
  std::map<DisplayLayoutModel::ViewPanelLayout, QAbstractButton *> rmap;
  rmap[DisplayLayoutModel::VIEW_ALL] = ui->btnFourViews;
  rmap[DisplayLayoutModel::VIEW_AXIAL] = ui->btnAxial;
  rmap[DisplayLayoutModel::VIEW_CORONAL] = ui->btnCoronal;
  rmap[DisplayLayoutModel::VIEW_SAGITTAL] = ui->btnSagittal;
  rmap[DisplayLayoutModel::VIEW_3D] = ui->btn3D;
  makeRadioGroupCoupling(ui->grpDisplayLayout, rmap,
                         m_Model->GetViewPanelLayoutModel());

  // Couple the radio buttons for layer layout
  makeRadioGroupCoupling(ui->grpLayerLayout,
                         m_Model->GetSliceViewLayerLayoutModel());
}
