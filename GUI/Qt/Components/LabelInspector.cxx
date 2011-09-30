#include "LabelInspector.h"
#include "ui_LabelInspector.h"
#include <QPainter>
#include <QPixmap>
#include "GlobalUIModel.h"

#include "IRISApplication.h"
#include "ColorLabelTable.h"
#include "QtDoubleSpinboxCoupling.h"

LabelInspector::LabelInspector(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::LabelInspector)
{
  ui->setupUi(this);

  ui->inForeLabel->setIconSize(QSize(ICON_SIZE,ICON_SIZE));
  ui->inBackLabel->setIconSize(QSize(ICON_SIZE,ICON_SIZE));
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
  makeCoupling(ui->inOpacity, m_Model->GetDriver()->GetGlobalState(),
               &GlobalState::SetSegmentationAlpha,
               &GlobalState::GetSegmentationAlpha);

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
  ui->inForeLabel->clear();
  ui->inBackLabel->clear();

  // Add initial entries to background
  QPixmap pix(ICON_SIZE, ICON_SIZE);
  pix.fill(QColor(0,0,0,0));

  ui->inBackLabel->addItem(QIcon(pix),"All labels",
                           QVariant((int) PAINT_OVER_ALL));

  ui->inBackLabel->addItem(QIcon(pix),"Non-hidden labels",
                           QVariant((int) PAINT_OVER_VISIBLE));

  // Create items for all labels
  ColorLabelTable::LabelMap lm =
      m_Model->GetDriver()->GetColorLabelTable()->GetValidLabels();

  QRect r(2,2,ICON_SIZE-5,ICON_SIZE-5);

  for(ColorLabelTable::LabelMapConstIterator it = lm.begin();
      it != lm.end(); ++it)
    {
    ColorLabel cl = it->second;

    // Create a pixmap
    QPixmap pix(ICON_SIZE, ICON_SIZE);
    pix.fill(QColor(0,0,0,0));
    QPainter paint(&pix);
    paint.setPen(Qt::black);
    paint.fillRect(r,QColor(cl.GetRGB(0), cl.GetRGB(1), cl.GetRGB(2)));
    paint.drawRect(r);
    QIcon ic(pix);

    int id = (int) it->first;
    ui->inBackLabel->addItem(ic, cl.GetLabel(), QVariant(PAINT_OVER_ONE + id));
    ui->inForeLabel->addItem(ic, cl.GetLabel(), QVariant(id));
    }
}

const int LabelInspector::ICON_SIZE = 16;

void LabelInspector::on_inForeLabel_currentIndexChanged(int index)
{
  GlobalState *gs = m_Model->GetDriver()->GetGlobalState();

  // Get the current index
  int label = ui->inForeLabel->itemData(index).toInt();
  if(label != gs->GetDrawingColorLabel())
    gs->SetDrawingColorLabel((LabelType) label);
}

void LabelInspector::on_inBackLabel_currentIndexChanged(int index)
{

}

void LabelInspector::on_inOpacity_valueChanged(int value)
{

}

void LabelInspector::on_btnEdit_clicked()
{

}

void LabelInspector::on_cbInvert_stateChanged(int arg1)
{

}
