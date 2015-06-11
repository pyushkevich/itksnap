#include "GeneralLayerInspector.h"
#include "ui_GeneralLayerInspector.h"

#include "LayerGeneralPropertiesModel.h"

#include "QtCheckBoxCoupling.h"
#include "QtComboBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtLineEditCoupling.h"

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
  makeCoupling(ui->chkAnimate, m_Model->GetAnimateModel());

  makeRadioGroupCoupling(ui->grpOverlayChecks, m_Model->GetIsStickyModel());

  makeCoupling(ui->inOpacity, m_Model->GetLayerOpacityModel());
  makeCoupling(ui->inOpacityValue, m_Model->GetLayerOpacityModel());
  makeCoupling(ui->chkVisible, m_Model->GetLayerVisibilityModel());

  makeCoupling(ui->outFilename, m_Model->GetFilenameModel());
  makeCoupling(ui->inNickname, m_Model->GetNicknameModel());

  activateOnFlag(ui->grpMulticomponent, m_Model,
                 LayerGeneralPropertiesModel::UIF_MULTICOMPONENT);

  activateOnFlag(ui->chkAnimate, m_Model,
                 LayerGeneralPropertiesModel::UIF_CAN_SWITCH_COMPONENTS);
  activateOnFlag(ui->inComponent, m_Model,
                 LayerGeneralPropertiesModel::UIF_CAN_SWITCH_COMPONENTS);
  activateOnFlag(ui->inComponentSlider, m_Model,
                 LayerGeneralPropertiesModel::UIF_CAN_SWITCH_COMPONENTS);
  activateOnFlag(ui->lblComponent, m_Model,
                 LayerGeneralPropertiesModel::UIF_CAN_SWITCH_COMPONENTS);

  activateOnFlag(ui->grpOverlay, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_STICKINESS_EDITABLE);

  activateOnFlag(ui->grpOpacity, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_OPACITY_EDITABLE);
  activateOnFlag(ui->lblOpacity, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_OPACITY_EDITABLE);
}
