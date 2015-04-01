#include "OverlayRolePage.h"
#include "ui_OverlayRolePage.h"
#include "QtWidgetCoupling.h"
#include "QtRadioButtonCoupling.h"
#include "QtComboBoxCoupling.h"
#include "SNAPQtCommon.h"
#include "GlobalUIModel.h"
#include "ColorMapModel.h"

namespace imageiowiz {

OverlayRolePage::OverlayRolePage(QWidget *parent) :
  AbstractPage(parent),
  ui(new Ui::OverlayRolePage)
{
  ui->setupUi(this);
}

OverlayRolePage::~OverlayRolePage()
{
  delete ui;
}


void OverlayRolePage::initializePage()
{
  this->setTitle("How should the image be displayed?");

  // Couple the radio buttons
  std::map<bool, QAbstractButton *> radioMap;
  radioMap[true] = ui->btnOverlay;
  radioMap[false] = ui->btnSeparate;
  makeRadioGroupCoupling(ui->wgtMain, radioMap, this->m_Model->GetStickyOverlayModel());

  // Couple the color map selector
  PopulateColorMapPresetCombo(ui->inOverlayColorMap,
                              this->m_Model->GetParent()->GetColorMapModel());

  makeCoupling(ui->inOverlayColorMap, this->m_Model->GetStickyOverlayColorMapModel());
}

bool OverlayRolePage::validatePage()
{
  return true;
}

bool OverlayRolePage::isComplete() const
{
  return true;
}


}
