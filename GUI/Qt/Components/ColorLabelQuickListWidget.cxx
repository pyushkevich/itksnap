#include "ColorLabelQuickListWidget.h"

#include <QToolBar>
#include <QAction>
#include <QActionGroup>
#include <QBoxLayout>

#include "GlobalUIModel.h"
#include "GlobalState.h"
#include "IRISApplication.h"
#include "ColorLabelQuickListModel.h"
#include "ColorLabelTable.h"
#include "SNAPQtCommon.h"
#include "QtToolbarCoupling.h"

ColorLabelQuickListWidget::ColorLabelQuickListWidget(QWidget *parent) :
  SNAPComponent(parent)
{
  // There is no model
  m_Model = NULL;

  // Create and configure the toolbar
  m_Toolbar = new QToolBar();
  m_Toolbar->setIconSize(QSize(16,16));
  m_Toolbar->setMovable(false);
  m_Toolbar->setFloatable(false);
  m_Toolbar->setStyleSheet("padding: 0px; spacing: 0px;");
  m_Toolbar->setContentsMargins(0,0,0,0);

  // Create and configure the action group
  m_ActionGroup = new QActionGroup(m_Toolbar);

  // Add toolbar via layout
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  layout->addWidget(m_Toolbar);
  this->setLayout(layout);

  // RE-emit the signal from the toolbar
  connect(m_Toolbar, SIGNAL(actionTriggered(QAction*)), this, SIGNAL(actionTriggered(QAction*)));

  // Define the maximum label count
  m_MaximumLabelCount = 6;
}

void ColorLabelQuickListWidget::SetModel(GlobalUIModel *model)
{
  // Grab a copy of the model
  m_Model = model;

  // Listen to changes in the label quicklist
  ColorLabelQuickListModel *qlm = m_Model->GetColorLabelQuickListModel();
  connectITK(qlm, ModelUpdateEvent());

  // Use the coupling mechanism to handle label selection.
  makeCoupling(m_Toolbar, qlm->GetActiveComboModel());
}

void ColorLabelQuickListWidget::setMaximumLabelCount(int value)
{
  // Change the maximum label count
  m_MaximumLabelCount = value;

  // We need to update the toolbar, but only if already there is a model
  if(m_Model)
    this->UpdateToolbar();
}

void ColorLabelQuickListWidget::onModelUpdate(const EventBucket &bucket)
{
  ColorLabelQuickListModel *qlm = m_Model->GetColorLabelQuickListModel();

  // If there is an event from the quick list model, we need to rebuild
  if(bucket.HasEvent(ModelUpdateEvent(), qlm))
    {
    this->UpdateToolbar();
    }
}

void ColorLabelQuickListWidget::UpdateToolbar()
{
  // Get the color label table
  ColorLabelTable *clt = m_Model->GetDriver()->GetColorLabelTable();

  // Update the source of the label quick-list
  ColorLabelQuickListModel *qlm = m_Model->GetColorLabelQuickListModel();
  qlm->Update();

  // Get the updated list of recent labels
  const ColorLabelQuickListModel::ComboList &qlist = qlm->GetRecentCombos();

  // Remove every action from the toolbar
  foreach(QAction *action, m_ActionGroup->actions())
    m_ActionGroup->removeAction(action);

  m_Toolbar->clear();

  // Add an action for each item
  for(int i = 0; i < qlist.size(); i++)
    {
    // Create the action
    QAction *action = new QAction(m_Toolbar);

    // Get the foreground/background
    LabelType fg = qlist[i].first;
    DrawOverFilter bg = qlist[i].second;

    // Configure action apperance
    action->setIcon(CreateLabelComboIcon(16, 16, fg, bg, clt));
    action->setToolTip(CreateLabelComboTooltip(fg, bg, clt));

    // Set the action's data
    action->setData(i);

    // Make action checkable
    action->setCheckable(true);

    // Add the action to the action group
    action->setActionGroup(m_ActionGroup);

    // Add the action to the toolbar
    m_Toolbar->addAction(action);
    }
}
