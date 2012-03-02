#include "LayerInspectorDialog.h"
#include "ui_LayerInspectorDialog.h"
#include "ContrastInspector.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "ImageWrapper.h"
#include "LayerSelectionModel.h"
#include "GenericImageData.h"
#include "IntensityCurveModel.h"



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
    return m_Model->GetNumberOfLayers();
  }

  QVariant data(const QModelIndex &index, int qtrole) const
  {
    // Ignore bad requests
    if(!index.isValid() || qtrole != Qt::DisplayRole)
      return QVariant();

    // Get the name of the layer
    LayerIterator it = m_Model->GetNthLayer(index.row());
    return QString(it.GetDynamicNickname().c_str());
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

private:
  LayerSelectionModel *m_Model;

};



LayerInspectorDialog::LayerInspectorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LayerInspectorDialog)
{
  ui->setupUi(this);
  m_LayerListModel = new LayerListQtModel(this);
  // ui->inLayer->setItemDelegate(new LayerListEditNameDelegate(this));
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

void LayerInspectorDialog::onLayerSelection()
{
  // Update the current layer selection
  QModelIndex idx = ui->inLayer->selectionModel()->currentIndex();
  GreyImageWrapperBase *layer =
      m_Model->GetLoadedLayersSelectionModel()->GetNthLayer(idx.row()).GetLayerAsGray();
  m_Model->GetIntensityCurveModel()->SetLayer(layer);
}
