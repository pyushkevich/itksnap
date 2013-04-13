#include "LabelSelectionPopup.h"
#include "ui_LabelSelectionPopup.h"

#include "GlobalUIModel.h"
#include <QToolBar>
#include <QStatusBar>
#include <SNAPQtCommon.h>
#include "IRISApplication.h"
#include "ColorLabelTable.h"
#include "GlobalState.h"
#include "QtComboBoxCoupling.h"
#include <QMenu>
#include <QValidator>

LabelSelectionPopup::LabelSelectionPopup(QWidget *parent) :
  SNAPComponent(parent),
  ui(new Ui::LabelSelectionPopup)
{
  ui->setupUi(this);

  m_ToolBarFore = new QToolBar(this);
  m_ToolBarFore->setIconSize(QSize(16,16));
  ui->tbForeground->layout()->addWidget(m_ToolBarFore);

  m_ToolBarBack = new QToolBar(this);
  m_ToolBarBack->setIconSize(QSize(16,16));
  ui->tbBackground->layout()->addWidget(m_ToolBarBack);

  this->setWindowFlags(Qt::Popup);

  connect(m_ToolBarFore, SIGNAL(actionTriggered(QAction*)),
          SLOT(onForegroundToolbarAction(QAction*)));
  connect(m_ToolBarBack, SIGNAL(actionTriggered(QAction*)),
          SLOT(onBackgroundToolbarAction(QAction*)));
  connect(ui->inBackground, SIGNAL(currentIndexChanged(int)), SLOT(close()));
  connect(ui->inForeground, SIGNAL(currentIndexChanged(int)), SLOT(close()));
}

LabelSelectionPopup::~LabelSelectionPopup()
{
  delete ui;
}

void LabelSelectionPopup::SetModel(GlobalUIModel *model)
{
  m_Model = model;

  makeCoupling(ui->inForeground, m_Model->GetGlobalState()->GetDrawingColorLabelModel());
  makeCoupling(ui->inBackground, m_Model->GetGlobalState()->GetDrawOverFilterModel());

  connectITK(m_Model->GetGlobalState()->GetDrawingColorLabelModel(), ValueChangedEvent());
  connectITK(m_Model->GetGlobalState()->GetDrawOverFilterModel(), ValueChangedEvent());
  connectITK(m_Model->GetGlobalState()->GetDrawingColorLabelModel(), DomainChangedEvent());
  connectITK(m_Model->GetGlobalState()->GetDrawOverFilterModel(), DomainChangedEvent());

  this->UpdateContents();
}

void LabelSelectionPopup::onModelUpdate(const EventBucket &bucket)
{
  this->UpdateContents();
}

void LabelSelectionPopup::onForegroundToolbarAction(QAction *action)
{
  int cl = action->data().toInt();
  m_Model->GetGlobalState()->SetDrawingColorLabel((LabelType) cl);
  this->close();
}

void LabelSelectionPopup::onBackgroundToolbarAction(QAction *action)
{
  DrawOverFilter dof = qvariant_cast<DrawOverFilter>(action->data());
  m_Model->GetGlobalState()->SetDrawOverFilter(dof);
  this->close();
}

// Update the contents of the toolbars and stuff
void LabelSelectionPopup::UpdateContents()
{
  ColorLabelTable *clt = m_Model->GetDriver()->GetColorLabelTable();

  // Fill out the foreground toolbar
  m_ToolBarFore->clear();

  // TODO: add the six labels that were last used
  int i = 0;
  std::vector<DrawOverFilter> doflist;
  doflist.push_back(DrawOverFilter(PAINT_OVER_ALL, 0));
  doflist.push_back(DrawOverFilter(PAINT_OVER_VISIBLE, 0));
  doflist.push_back(DrawOverFilter(PAINT_OVER_ONE, 0));

  while(m_ToolBarFore->actions().size() < 6 && i < MAX_COLOR_LABELS)
    {
    const ColorLabel &cl = clt->GetColorLabel(i);
    QAction *action = m_ToolBarFore->addAction(
          CreateColorBoxIcon(16, 16, GetBrushForColorLabel(i, clt)),
          GetTitleForColorLabel(i, clt));
    action->setData(QVariant(i));
    i = clt->FindNextValidLabel((LabelType) i, false);

    if(doflist.size() < 6)
      doflist.push_back(DrawOverFilter(PAINT_OVER_ONE, i));
    }

  m_ToolBarBack->clear();
  for(int j = 0; j < doflist.size(); j++)
    {
    QAction *action = m_ToolBarBack->addAction(
          CreateColorBoxIcon(16, 16, GetBrushForDrawOverFilter(doflist[j], clt)),
          GetTitleForDrawOverFilter(doflist[j], clt));
    action->setData(QVariant::fromValue(doflist[j]));
    }
}
