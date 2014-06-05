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
        ? QIcon()
        : CreateColorBoxIcon(16, 16, desc.Color);
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

  ui->tableVoxelUnderCursor->setAlternatingRowColors(true);
  ui->tableVoxelUnderCursor->setFixedWidth(160);
  ui->tableVoxelUnderCursor->setFixedHeight(120);
  ui->tableVoxelUnderCursor->setContextMenuPolicy(Qt::CustomContextMenu);

  m_ContextMenu = new QMenu(ui->tableVoxelUnderCursor);
  m_ContextMenu->addAction(ui->actionAutoContrast);

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

  makeMultiRowCoupling(ui->tableVoxelUnderCursor,
                       m_Model->GetVoxelAtCursorModel(),
                       LayerCurrentVoxelInfoTableWidgetRowTraits());
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

void CursorInspector::on_actionAutoContrast_triggered()
{
  LayerIterator it =
      m_Model->GetParent()->GetLoadedLayersSelectionModel()->GetNthLayer(m_PopupRow);

  if(it.GetLayer()->GetDisplayMapping()->GetIntensityCurve())
    {
    // Select the currently highlighted layer
    m_Model->GetParent()->GetIntensityCurveModel()->SetLayer(it.GetLayer());

    // Auto-adjust intensity in the selected layer
    m_Model->GetParent()->GetIntensityCurveModel()->OnAutoFitWindow();
    }
}
