#ifdef WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif //_WIN32_WINNT
#define _WIN32_WINNT	0x0501
#endif //WIN32

#include "CursorInspector.h"
#include "ui_CursorInspector.h"
#include <VoxelIntensityQTableModel.h>
#include <QSpinBox>
#include <GenericImageData.h>
#include <IRISApplication.h>
#include <CursorInspectionModel.h>
#include <GlobalUIModel.h>
#include <QtWidgetActivator.h>
#include <QMenu>
#include <IntensityCurveModel.h>
#include <LayerSelectionModel.h>
#include "SNAPQtCommon.h"

#include <QtSpinBoxCoupling.h>
#include <QtCheckBoxCoupling.h>
#include <QtLineEditCoupling.h>
#include <QtTableWidgetCoupling.h>
#include <QtWidgetArrayCoupling.h>

#include <QStandardItemModel>
#include <QPixmapCache>

/**
  This class provide the coupling properties for coupling the table of
  voxel intensities to QTableWidget
  */
class LayerCurrentVoxelInfoRowDescriptionTraits
{
public:
  typedef CurrentVoxelInfoItemSetDomain::ValueType ValueType;

  static QString GetText(ValueType value,
                         const LayerCurrentVoxelInfo &desc, int col)
  {
    if(col == 0)
      return from_utf8(desc.LayerName);
    else
      return from_utf8(desc.IntensityValue);
  }

  static QIcon GetIcon(ValueType value,
                       const LayerCurrentVoxelInfo &desc, int col)
  {
    return (col == 0)
        ? QIcon(":/root/icons8_pin_16.png")
        : CreateColorBoxIcon(12, 12, desc.Color);
  }

  static QVariant GetIconSignature(ValueType value,
                                   const LayerCurrentVoxelInfo &desc, int col)
  {
    // Get the RGB color
    return (col == 0)
        ? QVariant(0)
        : QVariant(QColor(desc.Color[0], desc.Color[1], desc.Color[2]));
  }
};

typedef TextAndIconTableWidgetRowTraits<
  size_t, LayerCurrentVoxelInfo,
  LayerCurrentVoxelInfoRowDescriptionTraits> LayerCurrentVoxelInfoTableWidgetRowTraits;

/*
typedef ItemSetWidgetDomainTraits<
  CurrentVoxelInfoItemSetDomain,
  QTableWidget,
  LayerCurrentVoxelInfoTableWidgetRowTraits> LayerCurrentVoxelInfoDomainTraits;
*/






CursorInspector::CursorInspector(QWidget *parent) :
    SNAPComponent(parent),
    ui(new Ui::CursorInspector)
{
  ui->setupUi(this);

  // Create an empty standard item model
  m_CurrentVoxelItemModel = new QStandardItemModel(this);
  m_CurrentVoxelItemModel->setColumnCount(2);
  m_CurrentVoxelItemModel->setRowCount(1);

  // Set the header labels
  QStringList header;
  header << "Layer" << "Intensity";
  m_CurrentVoxelItemModel->setHorizontalHeaderLabels(header);

  ui->tableVoxelUnderCursor->setModel(m_CurrentVoxelItemModel);
  ui->tableVoxelUnderCursor->setAlternatingRowColors(true);
  ui->tableVoxelUnderCursor->setFixedWidth(160);
  ui->tableVoxelUnderCursor->setFixedHeight(120);
  ui->tableVoxelUnderCursor->setContextMenuPolicy(Qt::CustomContextMenu);

  ui->tableVoxelUnderCursor->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  ui->tableVoxelUnderCursor->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  ui->tableVoxelUnderCursor->setColumnWidth(0, 92);
  // ui->tableVoxelUnderCursor->setColumnWidth(1, 68);

  // Hook up the context menu
  connect(ui->tableVoxelUnderCursor, SIGNAL(customContextMenuRequested(QPoint)),
          SLOT(onContextMenuRequested(QPoint)));
}

CursorInspector::~CursorInspector()
{
  delete ui;
}

void CursorInspector::SetModel(CursorInspectionModel *model)
{
  m_Model = model;

  // Activators
  activateOnFlag(this, m_Model->GetParent(), UIF_BASEIMG_LOADED);

  // Couple to the model
  makeCoupling(ui->outLabelId, m_Model->GetLabelUnderTheCursorIdModel());
  makeCoupling(ui->outLabelText, m_Model->GetLabelUnderTheCursorTitleModel());

  makeArrayCoupling(ui->inCursorX, ui->inCursorY, ui->inCursorZ,
                    m_Model->GetCursorPositionModel());

  connectITK(m_Model->GetVoxelAtCursorModel(), DomainChangedEvent());
  connectITK(m_Model->GetVoxelAtCursorModel(), DomainDescriptionChangedEvent());
}

#include "MainImageWindow.h"
#include "LayerInspectorDialog.h"

void CursorInspector::onContextMenuRequested(QPoint pos)
{
  m_PopupRow = ui->tableVoxelUnderCursor->rowAt(pos.y());
  if(m_PopupRow >= 0)
    {
    // Instead of creating a separate context menu here, we use a context menu
    // from the corresponding row in the LayerInspector.
    MainImageWindow *winmain = findParentWidget<MainImageWindow>(this);
    LayerInspectorDialog *inspector = winmain->GetLayerInspector();

    // Find the corresponding layer
    LayerIterator it =
        m_Model->GetParent()->GetLoadedLayersSelectionModel()->GetNthLayer(m_PopupRow);

    // Get the menu
    QMenu *menu = inspector->GetLayerContextMenu(it.GetLayer());

    // Show the menu
    if(menu)
      menu->popup(QCursor::pos());
    }
}

void CursorInspector::UpdateVoxelTableRow(int i, const LayerCurrentVoxelInfo &vi)
{
  // Get the two items to update
  QStandardItem *item_layer = m_CurrentVoxelItemModel->item(i, 0);
  QStandardItem *item_intensity = m_CurrentVoxelItemModel->item(i, 1);

  item_layer->setText(from_utf8(vi.LayerName));
  item_intensity->setText(from_utf8(vi.IntensityValue));
  item_intensity->setToolTip(from_utf8(vi.IntensityValue));

  // item_layer->setForeground(QBrush(QColor(Qt::darkGray)));
  // item_intensity->setForeground(QBrush(QColor(Qt::darkGray)));

  QString tooltip_annot;

  // By default the color of the items is black
  item_layer->setForeground(QBrush(QColor(Qt::black)));
  item_intensity->setForeground(QBrush(QColor(Qt::black)));
  item_layer->setIcon(QIcon());

  if(vi.isSelectedGroundLayer)
    {
    // item_layer->setIcon(QIcon(":/root/icons8_star_8.png"));
    QFont font = item_layer->font(); font.setBold(true); font.setItalic(false);
    item_layer->setFont(font);
    item_layer->setToolTip(from_utf8(vi.LayerName));
    }
  else if(vi.isSticky)
    {
    // item_layer->setIcon(QIcon(":/root/icons8_pin_10.png"));
    QFont font = item_layer->font(); font.setBold(false); font.setItalic(true);
    item_layer->setFont(font);
    item_layer->setToolTip(QString("<p>%1</p><p>%2</p>").arg(
                             from_utf8(vi.LayerName)).arg(
                             "This layer is rendered as an overlay on top of other layers."));
    }
  else
    {
    QFont font = item_layer->font(); font.setBold(false); font.setItalic(false);
    item_layer->setFont(font);
    item_layer->setToolTip(from_utf8(vi.LayerName));
    }

  // Set the tooltip
  item_layer->setToolTip(
        QString("<p>%1</p><p>%2</p>").arg(from_utf8(vi.LayerName)).arg(tooltip_annot));

  // Set the color icon
  QColor stored_color = qvariant_cast<QColor>(item_intensity->data(Qt::UserRole));
  QColor new_color = QColor(vi.Color[0], vi.Color[1], vi.Color[2]);
  if(new_color != stored_color)
    {
    item_intensity->setIcon(CreateColorBoxIcon(12, 12, vi.Color));
    item_intensity->setData(new_color, Qt::UserRole);
    }
}

void CursorInspector::RebuildVoxelTable()
{
  // Initialize the model
  m_CurrentVoxelItemModel->removeRows(0, m_CurrentVoxelItemModel->rowCount());

  // Get the domain from which we are building this model
  int dummy;
  CurrentVoxelInfoItemSetDomain domain;
  if(m_Model->GetVoxelAtCursorModel()->GetValueAndDomain(dummy, &domain))
    {
    // Add the rows
    for(LayerIterator it = domain.begin(); it != domain.end(); ++it)
      {
      QList<QStandardItem *> items;
      items.push_back(new QStandardItem());
      items.push_back(new QStandardItem());
      m_CurrentVoxelItemModel->appendRow(items);
      this->UpdateVoxelTableRow(m_CurrentVoxelItemModel->rowCount()-1, domain.GetDescription(it));
      }
    }
}

void CursorInspector::UpdateVoxelTable()
{
  // Get the domain from which we are building this model
  int dummy;
  CurrentVoxelInfoItemSetDomain domain;
  if(m_Model->GetVoxelAtCursorModel()->GetValueAndDomain(dummy, &domain))
    {
    // Update the rows
    int row = 0;
    for(LayerIterator it = domain.begin(); it != domain.end(); ++it, ++row)
      {
      this->UpdateVoxelTableRow(row, domain.GetDescription(it));
      }
    }
}

void CursorInspector::onModelUpdate(const EventBucket &bucket)
{
  m_Model->Update();

  if(bucket.HasEvent(DomainChangedEvent(), m_Model->GetVoxelAtCursorModel()))
    {
    this->RebuildVoxelTable();
    }
  else if(bucket.HasEvent(DomainDescriptionChangedEvent(), m_Model->GetVoxelAtCursorModel()))
    {
    this->UpdateVoxelTable();
    }
}

void CursorInspector::on_tableVoxelUnderCursor_clicked(const QModelIndex &index)
{
  // When the user clicks on an item, that item will become the visible one
  int item_row = index.row();
  if(item_row >= 0)
    {
    // Find the corresponding layer
    LayerIterator it =
        m_Model->GetParent()->GetLoadedLayersSelectionModel()->GetNthLayer(item_row);

    if(!it.GetLayer()->IsSticky())
      m_Model->GetParent()->GetDriver()->GetGlobalState()->SetSelectedLayerId(
            it.GetLayer()->GetUniqueId());
    }

}
