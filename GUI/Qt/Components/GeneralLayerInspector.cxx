#include "GeneralLayerInspector.h"
#include "ui_GeneralLayerInspector.h"

#include "LayerGeneralPropertiesModel.h"

#include "QtCheckBoxCoupling.h"
#include "QtComboBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtLineEditCoupling.h"
#include "QtAbstractButtonCoupling.h"

#include "QtWidgetActivator.h"
#include "QtRadioButtonCoupling.h"

Q_DECLARE_METATYPE(LayerGeneralPropertiesModel::DisplayMode)

GeneralLayerInspector::GeneralLayerInspector(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::GeneralLayerInspector)
{
  ui->setupUi(this);
}

GeneralLayerInspector::~GeneralLayerInspector()
{
  delete ui;
}

void GeneralLayerInspector::SetModel(LayerGeneralPropertiesModel *model)
{
  m_Model = model;

  // Couple the widgets
  makeCoupling(ui->inMode, m_Model->GetDisplayModeModel());
  makeCoupling(ui->inComponent, m_Model->GetSelectedComponentModel());
  makeCoupling(ui->inComponentSlider, m_Model->GetSelectedComponentModel());
  makeCoupling((QAbstractButton *)ui->btnAnimate, m_Model->GetAnimateModel());

  // Couple the pin/unpin buttons
  std::map<bool, QAbstractButton *> button_map;
  button_map[true] = ui->btnPin;
  button_map[false] = ui->btnUnpin;
  makeCheckableWidgetGroupCoupling(ui->grpOverlayChecks, button_map, m_Model->GetIsStickyModel());

  makeCoupling(ui->inOpacity, m_Model->GetLayerOpacityModel());
  makeCoupling(ui->inOpacityValue, m_Model->GetLayerOpacityModel());
  makeCoupling((QAbstractButton *)ui->btnVisible, m_Model->GetLayerVisibilityModel());

  makeCoupling(ui->outFilename, m_Model->GetFilenameModel());
  makeCoupling(ui->inNickname, m_Model->GetNicknameModel());

  activateOnFlag(ui->grpMulticomponent, m_Model,
                 LayerGeneralPropertiesModel::UIF_MULTICOMPONENT,
                 QtWidgetActivator::HideInactive);

  activateOnFlag(ui->grpComponent, m_Model,
                 LayerGeneralPropertiesModel::UIF_CAN_SWITCH_COMPONENTS);

  activateOnFlag(ui->grpSecondary, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_STICKINESS_EDITABLE,
                 QtWidgetActivator::HideInactive);

  activateOnFlag(ui->grpOpacity, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_OPACITY_EDITABLE);
  activateOnFlag(ui->lblOpacity, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_OPACITY_EDITABLE);

  activateOnFlag(ui->btnUp, m_Model,
                 LayerGeneralPropertiesModel::UIF_MOVABLE_UP);
  activateOnFlag(ui->btnDown, m_Model,
                 LayerGeneralPropertiesModel::UIF_MOVABLE_DOWN);
}

void GeneralLayerInspector::on_btnUp_clicked()
{
  m_Model->MoveLayerUp();
}

void GeneralLayerInspector::on_btnDown_clicked()
{
  m_Model->MoveLayerDown();
}
