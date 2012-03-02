#include "LayerInspectorDialog.h"
#include "ui_LayerInspectorDialog.h"
#include "ContrastInspector.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "ImageWrapper.h"
#include "LayerSelectionModel.h"
#include "GenericImageData.h"

/**
  Model that handles interaction with the list of layers GUI
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
    return m_Model->GetNumberOfLayers();
  }

  QVariant data(const QModelIndex &index, int qtrole) const
  {
    // Ignore bad requests
    if(!index.isValid() || qtrole != Qt::DisplayRole)
      return QVariant();

    // Get the name of the layer
    LayerIterator::LayerRole role = m_Model->GetRoleOfNthLayer(index.row());

    if(role == LayerIterator::MAIN_ROLE)
      return QVariant("Main Image");
    else if(role == LayerIterator::OVERLAY_ROLE)
      return QVariant("Overlay");
    else return QVariant();
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


}

ContrastInspector * LayerInspectorDialog::GetContrastInspector()
{
  return ui->cmpInspector;
}

#include "IntensityCurveModel.h"

void LayerInspectorDialog::onLayerSelection()
{
  // Update the current layer selection
  QModelIndex idx = ui->inLayer->selectionModel()->currentIndex();
  GreyImageWrapperBase *layer =
      m_Model->GetLoadedLayersSelectionModel()->GetNthLayerAsGrey(idx.row());
  m_Model->GetIntensityCurveModel()->SetLayer(layer);
}
