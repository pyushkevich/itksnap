#include "LabelInspector.h"
#include "ui_LabelInspector.h"
#include <QPainter>
#include <QPixmap>
#include <SNAPQtCommon.h>
#include "GlobalUIModel.h"

#include "IRISApplication.h"
#include "ColorLabelTable.h"
#include "QtWidgetCoupling.h"

LabelInspector::LabelInspector(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::LabelInspector)
{
  ui->setupUi(this);

  ui->inForeLabel->setIconSize(QSize(ICON_SIZE,ICON_SIZE));
  ui->inBackLabel->setIconSize(QSize(ICON_SIZE,ICON_SIZE));

  m_IsFillingCombos = false;
}

LabelInspector::~LabelInspector()
{
  delete ui;
}

void LabelInspector
::onModelUpdate(const EventBucket &bucket)
{
  m_Model->Update();
}

void LabelInspector
::SetModel(GlobalUIModel *model)
{
  // Get the model
  m_Model = model;

  // Use couplings where we can
  makeCoupling(ui->inOpacity,
               m_Model->GetDriver()->GetGlobalState()->GetSegmentationAlphaModel());

  FillCombos();
  UpdateValuesFromModel();
}

#include <QLinearGradient>

void LabelInspector::UpdateValuesFromModel()
{
  GlobalState *gs = m_Model->GetDriver()->GetGlobalState();

  // Update the current label
  LabelType fg = gs->GetDrawingColorLabel();
  LabelType bg = gs->GetOverWriteColorLabel();
  CoverageModeType cm = gs->GetCoverageMode();
  int sel = ((int) cm) + bg;

  ui->inForeLabel->setCurrentIndex(
        ui->inForeLabel->findData(QVariant((int) fg)));

  ui->inBackLabel->setCurrentIndex(
        ui->inBackLabel->findData(QVariant(sel)));

  // Update the checkbox
  ui->cbInvert->setChecked(gs->GetPolygonInvert());

  // Update the opacity
  // ui->inOpacity->setValue(gs->GetSegmentationAlpha());
}

void LabelInspector::FillCombos()
{
  m_IsFillingCombos = true;

  ui->inForeLabel->clear();
  ui->inBackLabel->clear();

  // Add initial entries to background
  QPixmap pix(ICON_SIZE, ICON_SIZE);
  pix.fill(QColor(0,0,0,0));

  ui->inBackLabel->addItem(QIcon(pix),"All labels",
                           QVariant((int) PAINT_OVER_ALL));

  ui->inBackLabel->addItem(QIcon(pix),"Non-hidden labels",
                           QVariant((int) PAINT_OVER_VISIBLE));

  // Get the color label table
  ColorLabelTable *clt = m_Model->GetDriver()->GetColorLabelTable();

    for(ColorLabelTable::ValidLabelConstIterator it = clt->begin();
      it != clt->end(); ++it)
    {
    ColorLabel cl = it->second;

    // Create a pixmap
    QIcon ic = CreateColorBoxIcon(
          ICON_SIZE, ICON_SIZE,
          Vector3ui(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2)));

    int id = (int) it->first;
    ui->inBackLabel->addItem(ic, cl.GetLabel(), QVariant(PAINT_OVER_ONE + id));
    ui->inForeLabel->addItem(ic, cl.GetLabel(), QVariant(id));
    }

  m_IsFillingCombos = false;
}

const int LabelInspector::ICON_SIZE = 16;

void LabelInspector::on_inForeLabel_currentIndexChanged(int index)
{
  if(!m_IsFillingCombos)
    {
    GlobalState *gs = m_Model->GetDriver()->GetGlobalState();

    // Get the current index
    int label = ui->inForeLabel->itemData(index).toInt();
    if(label != gs->GetDrawingColorLabel())
      gs->SetDrawingColorLabel((LabelType) label);
    }
}

void LabelInspector::on_inBackLabel_currentIndexChanged(int index)
{
  if(!m_IsFillingCombos)
    {
    GlobalState *gs = m_Model->GetDriver()->GetGlobalState();

    // Get the current index
    int label = ui->inBackLabel->itemData(index).toInt();

    // Handle the special cases
    if(label == 0)
      {
      gs->SetCoverageMode(PAINT_OVER_ALL);
      gs->SetOverWriteColorLabel(0);
      }
    else if(label == 1)
      {
      gs->SetCoverageMode(PAINT_OVER_VISIBLE);
      gs->SetOverWriteColorLabel(0);
      }
    else
      {
      gs->SetCoverageMode(PAINT_OVER_ONE);
      gs->SetOverWriteColorLabel(label - 2);
      }
    }
}

void LabelInspector::on_inOpacity_valueChanged(int value)
{

}

#include <LabelEditorDialog.h>

void LabelInspector::on_btnEdit_clicked()
{
  LabelEditorDialog *led = new LabelEditorDialog(this);
  led->SetModel(this->m_Model->GetLabelEditorModel());
  led->exec();
}


