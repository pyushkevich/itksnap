#include "GeneralLayerInspector.h"
#include "ui_GeneralLayerInspector.h"

#include "LayerGeneralPropertiesModel.h"

#include "QtCheckBoxCoupling.h"
#include "QtComboBoxCoupling.h"
#include "QtSpinBoxCoupling.h"
#include "QtSliderCoupling.h"
#include "QtLineEditCoupling.h"
#include "QtAbstractButtonCoupling.h"
#include "TagListWidgetCoupling.h"
#include "MeshWrapperBase.h"
#include "StandaloneMeshWrapper.h"

#include "QtWidgetActivator.h"
#include "QtRadioButtonCoupling.h"

#include "TagListWidget.h"

Q_DECLARE_METATYPE(LayerGeneralPropertiesModel::DisplayMode)

using MeshDataType = LayerGeneralPropertiesModel::MeshDataType;

std::map<int, const char*>
GeneralLayerInspector::m_MeshDataTypeToIcon =
{
	{ MeshDataType::POINT_DATA, ":/root/mesh_point_data.png" },
	{ MeshDataType::CELL_DATA, ":/root/mesh_cell_data.png" }
};

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
  makeCoupling(ui->sliderTP, m_Model->GetParentModel()->GetCursorTimePointModel());
  makeCoupling(ui->spinBoxTP, m_Model->GetParentModel()->GetCursorTimePointModel());
  makeCoupling(ui->inTPNickname, m_Model->GetCrntTimePointNicknameModel());
  makeCoupling(ui->TPTagsWidget, m_Model->GetCrntTimePointTagListModel());
  makeCoupling((QAbstractButton *)ui->btn4DReplay, m_Model->GetParentModel()->GetGlobalState()->Get4DReplayModel());
  makeCoupling(ui->in4DReplayInterval, m_Model->GetParentModel()->GetGlobalState()->Get4DReplayIntervalModel());

  auto validator = new QIntValidator(this);
  validator->setRange(1, 60000); // who would need >1min interval?
  ui->in4DReplayInterval->setValidator(validator);

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

  makeCoupling(ui->tagsWidget, m_Model->GetTagsModel());

  activateOnFlag(ui->grpMulticomponent, m_Model,
                 LayerGeneralPropertiesModel::UIF_MULTICOMPONENT,
                 QtWidgetActivator::HideInactive);

  activateOnFlag(ui->grp4DProperties, m_Model,
                 LayerGeneralPropertiesModel::UIF_IS_4D_IMAGE,
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

	// Group Mesh should only display when a mesh layer with data is selected
  activateOnFlag(ui->grpMesh, m_Model,
								 LayerGeneralPropertiesModel::UIF_MESH_HAS_DATA,
                 QtWidgetActivator::HideInactive);

	// Mesh data array display connection
  LatentITKEventNotifier::connect(
        m_Model, ActiveLayerChangeEvent(), this, SLOT(meshData_domainChanged()));

	QObject::connect(ui->boxMeshDataName, SIGNAL(currentIndexChanged(int)),
									 this, SLOT(meshData_selectionChanged(int)));

	// Mesh vector data connection
	activateOnFlag(ui->grpMeshVectorMode, m_Model,
								 LayerGeneralPropertiesModel::UIF_IS_MESHDATA_MULTICOMPONENT,
								 QtWidgetActivator::HideInactive);

	makeCoupling(ui->boxMeshVectorMode, m_Model->GetMeshVectorModeModel());


}

void GeneralLayerInspector::on_btnUp_clicked()
{
  m_Model->MoveLayerUp();
}

void GeneralLayerInspector::on_btnDown_clicked()
{
  m_Model->MoveLayerDown();
}

void GeneralLayerInspector::on_spinBoxTP_valueChanged(int value)
{
  QString txt("Properties for Time Point ");
  txt.append(std::to_string(value).c_str());
  txt.append(":");
  ui->grpTPProperties->setTitle(txt);
}

void
GeneralLayerInspector
::meshData_domainChanged()
{
	auto mesh = dynamic_cast<StandaloneMeshWrapper*>(m_Model->GetLayer());

  if (mesh && mesh->GetUniqueId() != this->m_CurrentlyActiveMeshLayerId)
		{
    this->m_CurrentlyActiveMeshLayerId = mesh->GetUniqueId();

		auto boxMetaName = ui->boxMeshDataName;
		boxMetaName->clear();

    //auto &propMap = mesh->GetCombinedDataProperty();
    LayerGeneralPropertiesModel::MeshLayerDataPropertiesMap propMap;
    if (m_Model->GetMeshDataArrayPropertiesMap(propMap))
      {
      for (auto &kv : propMap)
        {
        boxMetaName->addItem(QIcon(m_MeshDataTypeToIcon[kv.second->GetType()]),
            kv.second->GetName(), kv.first);
        }
      ui->boxMeshDataName->setCurrentIndex(mesh->GetActiveMeshLayerDataPropertyId());
      }
		}
}

void
GeneralLayerInspector
::meshData_selectionChanged(int value)
{
  m_Model->SetActiveMeshLayerDataPropertyId(value);
}

void GeneralLayerInspector::onModelUpdate(const EventBucket &)
{
}
