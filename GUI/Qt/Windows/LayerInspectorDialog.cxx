#include "LayerInspectorDialog.h"
#include "ui_LayerInspectorDialog.h"
#include "ContrastInspector.h"
#include "GlobalUIModel.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "ImageWrapper.h"
#include "LayerSelectionModel.h"
#include "GenericImageData.h"
#include "IntensityCurveModel.h"
#include "ColorMapModel.h"
#include "ImageInfoModel.h"
#include "LatentITKEventNotifier.h"
#include "QtDoubleSliderWithEditorCoupling.h"
#include "ComponentSelectionModel.h"




/**
  Model that handles interaction with the list of layers GUI. For the time
  being, this only handles the nickname of the layers, but it could also
  provide support for overall visibility, other aspects.
  */
class LayerListQtModel : public QAbstractListModel
{
public:
  LayerListQtModel(QWidget *parent)
    : QAbstractListModel(parent) {}
  virtual ~LayerListQtModel() {}

  void SetParentModel(LayerSelectionModel *model)
  {
    m_Model = model;
  }

  int rowCount(const QModelIndex &parent) const
  {
    int rc = m_Model->GetNumberOfLayers();
    return rc;
  }

  QVariant data(const QModelIndex &index, int qtrole) const
  {
    // Ignore bad requests
    if(!index.isValid())
      return QVariant();

    if(qtrole == Qt::DisplayRole || qtrole == Qt::EditRole)
      {
      // Get the name of the layer
      LayerIterator it = m_Model->GetNthLayer(index.row());
      return QString(it.GetDynamicNickname().c_str());
      }
    else if(qtrole == Qt::ToolTipRole)
      {
      // Get the name of the layer
      LayerIterator it = m_Model->GetNthLayer(index.row());
      return QString(it.GetLayer()->GetFileName());
      }
    else
      return QVariant();
  }

  bool setData(const QModelIndex &index, const QVariant &value, int qtrole)
  {
    // Ignore bad requests
    if(!index.isValid() || qtrole != Qt::EditRole)
      return false;

    // Get the layer iter
    LayerIterator it = m_Model->GetNthLayer(index.row());
    QString nick(it.GetDynamicNickname().c_str());
    if(nick != value.toString())
      {
      it.GetLayer()->SetNickname(value.toString().toStdString());
      emit dataChanged(index, index);
      return true;
      }
    return false;
  }

  Qt::ItemFlags flags(const QModelIndex &index) const
  {
    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
  }

  void onLayerListUpdate()
  {
    emit layoutChanged();
  }

private:
  LayerSelectionModel *m_Model;

};



LayerInspectorDialog::LayerInspectorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LayerInspectorDialog)
{
  ui->setupUi(this);
  m_LayerListModel = new LayerListQtModel(this);
}

LayerInspectorDialog::~LayerInspectorDialog()
{
  delete ui;
}

void LayerInspectorDialog::SetModel(GlobalUIModel *model)
{
  this->m_Model = model;
  m_LayerListModel->SetParentModel(model->GetLoadedLayersSelectionModel());

  // Hook the model up to the layer list widget
  ui->inLayer->setModel(m_LayerListModel);
  connect(ui->inLayer->selectionModel(),
          SIGNAL(currentChanged(QModelIndex,QModelIndex)),
          this, SLOT(onLayerSelection()));

  // Hook up the model to the component inspectors
  ui->cmpContrast->SetModel(model->GetIntensityCurveModel());
  ui->cmpColorMap->SetModel(model->GetColorMapModel());
  ui->cmpInfo->SetModel(model->GetImageInfoModel());
  ui->cmpMetadata->SetModel(model->GetImageInfoModel());
  ui->cmpComponent->SetModel(model->GetComponentSelectionModel());

  // We need to listen to layer changes in the model
  LatentITKEventNotifier::connect(
        model, LayerChangeEvent(),
        this, SLOT(onModelUpdate(const EventBucket &)));

  // Connect the layer opacity model
  makeCoupling(
        ui->inLayerOpacity,
        model->GetColorMapModel()->GetLayerOpacityModel());
}

void LayerInspectorDialog::onLayerSelection()
{
  // Update the current layer selection
  QModelIndex idx = ui->inLayer->selectionModel()->currentIndex();
  LayerIterator it = m_Model->GetLoadedLayersSelectionModel()->GetNthLayer(idx.row());
  ImageWrapperBase *layer = it.GetLayer();

  // For each model, set the layer
  m_Model->GetColorMapModel()->SetLayer(
    layer->GetDisplayMapping()->GetColorMap() ? layer : NULL);

  m_Model->GetIntensityCurveModel()->SetLayer(
        layer->GetDisplayMapping()->GetIntensityCurve() ? layer : NULL);

  m_Model->GetImageInfoModel()->SetLayer(layer);
  m_Model->GetComponentSelectionModel()->SetLayer(it.GetLayerAsVector());
}

void LayerInspectorDialog::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(LayerChangeEvent()))
    {
    // If the layers have changed, the qt model needs to be refreshed
    m_LayerListModel->onLayerListUpdate();

    // TODO: this is the wrong place for this!!!
    m_Model->GetImageInfoModel()->Update();
    m_Model->GetColorMapModel()->Update();
    m_Model->GetIntensityCurveModel()->Update();
    m_Model->GetComponentSelectionModel()->Update();

    // If the currently selected layer has been lost, move to the first
    // available layer
    if(m_Model->GetIntensityCurveModel()->GetLayer() == NULL)
      {
      if(m_Model->GetLoadedLayersSelectionModel()->GetNumberOfLayers() > 0)
        {
        ui->inLayer->selectionModel()->setCurrentIndex(
              m_LayerListModel->index(0),
              QItemSelectionModel::SelectCurrent);
        }
      }

    // Layer selection needs to be updated
    onLayerSelection();
    }
}
