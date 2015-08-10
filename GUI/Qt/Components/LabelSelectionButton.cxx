#include "LabelSelectionButton.h"
#include "GlobalUIModel.h"
#include "AbstractModel.h"
#include "LatentITKEventNotifier.h"
#include "GlobalState.h"
#include "IRISApplication.h"
#include "ColorLabelTable.h"
#include <QtComboBoxCoupling.h>

#include "SNAPQtCommon.h"
#include <QPainter>
#include <QIcon>
#include <QColor>
#include <QMenu>

LabelSelectionButton::LabelSelectionButton(QWidget *parent) :
  QToolButton(parent)
{
  LabelSelectionButtonPopupMenu *menu = new LabelSelectionButtonPopupMenu(this);
  this->setMenu(menu);
  this->setPopupMode(QToolButton::InstantPopup);
  this->setIconSize(QSize(22,22));
}


void LabelSelectionButton::SetModel(GlobalUIModel *model)
{
  m_Model = model;

  LatentITKEventNotifier::connect(
        m_Model->GetGlobalState()->GetDrawingColorLabelModel(),
        IRISEvent(), this, SLOT(onModelUpdate(const EventBucket&)));

  LatentITKEventNotifier::connect(
        m_Model->GetGlobalState()->GetDrawOverFilterModel(),
        IRISEvent(), this, SLOT(onModelUpdate(const EventBucket&)));

  LabelSelectionButtonPopupMenu *menu
      = static_cast<LabelSelectionButtonPopupMenu *>(this->menu());
  menu->SetModel(model);

  this->UpdateAppearance();
}

void LabelSelectionButton::onModelUpdate(const EventBucket &bucket)
{
  this->UpdateAppearance();
}


void LabelSelectionButton::UpdateAppearance()
{
  ColorLabelTable *clt = m_Model->GetDriver()->GetColorLabelTable();
  LabelType fg = m_Model->GetGlobalState()->GetDrawingColorLabel();
  DrawOverFilter bg = m_Model->GetGlobalState()->GetDrawOverFilter();

  // Draw a split button
  this->setIcon(CreateLabelComboIcon(20, 20, fg, bg, clt));

  // Update the tooltip
  QString tooltip = this->toolTip();
  tooltip.replace(QRegularExpression("<!--FgStart-->.*<!--FgEnd-->"),
                  QString("<!--FgStart-->%1<!--FgEnd-->").arg(GetTitleForColorLabel(fg, clt)));
  tooltip.replace(QRegularExpression("<!--BgStart-->.*<!--BgEnd-->"),
                  QString("<!--BgStart-->%1<!--BgEnd-->").arg(GetTitleForDrawOverFilter(bg, clt)));
  this->setToolTip(tooltip);
}


/*
void LabelSelectionButton::UpdateAppearanceOld()
{
  int w = 16;

  QPixmap pm(w, w);
  QPainter qp(&pm);

  QPolygon poly_fg, poly_bg;
  poly_fg << QPoint(0, 0) << QPoint(w-1,0) << QPoint(0,w-1) << QPoint(0,0);
  poly_bg << QPoint(0, 0) << QPoint(w-1,0) << QPoint(w-1,w-1) << QPoint(0,w-1) << QPoint(0,0);

  LabelType fg = m_Model->GetGlobalState()->GetDrawingColorLabel();
  DrawOverFilter bg = m_Model->GetGlobalState()->GetDrawOverFilter();
  ColorLabel lfg = m_Model->GetDriver()->GetColorLabelTable()->GetColorLabel(fg);
  ColorLabel lbg = m_Model->GetDriver()->GetColorLabelTable()->GetColorLabel(bg.DrawOverLabel);

  QBrush brush_fg(QColor(lfg.GetRGB(0), lfg.GetRGB(1), lfg.GetRGB(2)));
  QBrush brush_bg(QColor(lbg.GetRGB(0), lbg.GetRGB(1), lbg.GetRGB(2)));

  qp.setPen(Qt::black);
  qp.setBrush(brush_bg);
  qp.drawPolygon(poly_bg);

  qp.setBrush(brush_fg);
  qp.drawPolygon(poly_fg);

  // Draw a split button
  this->setIcon(QIcon(pm));
}
*/

#include "ColorLabelQuickListWidget.h"
#include <QWidgetAction>
#include <QVBoxLayout>
#include <QLabel>

class ColorLabelQuickListWidgetAction : public QWidgetAction
{
public:
  ColorLabelQuickListWidgetAction(QWidget *parent)
    : QWidgetAction(parent)
  {
    QWidget *topWidget = new QWidget(parent);
    QVBoxLayout *lo = new QVBoxLayout(topWidget);
    lo->setContentsMargins(4,4,4,4);
    lo->setSpacing(2);

    m_Widget = new ColorLabelQuickListWidget(parent);

    lo->addWidget(new QLabel("Quick palette:"),0,Qt::AlignLeft);
    lo->addWidget(m_Widget, 0, Qt::AlignCenter);

    this->setDefaultWidget(topWidget);
  }

  irisGetMacro(Widget, ColorLabelQuickListWidget *)

protected:
  ColorLabelQuickListWidget *m_Widget;
};


LabelSelectionButtonPopupMenu::LabelSelectionButtonPopupMenu(QWidget *parent)
  : QMenu(parent)
{
  // Add the foreground and background label selectors
  m_SubForeground = this->addMenu("Active label:");
  m_SubBackground = this->addMenu("Paint over:");
  this->addSeparator();

  // Create a QAction wrapped around the recent labels menu
  ColorLabelQuickListWidgetAction *action = new ColorLabelQuickListWidgetAction(this);
  this->m_Recent = action->GetWidget();
  this->addAction(action);

  this->setStyleSheet("font-size: 12px;");

  connect(m_SubForeground, SIGNAL(triggered(QAction*)), SLOT(onForegroundAction(QAction*)));
  connect(m_SubBackground, SIGNAL(triggered(QAction*)), SLOT(onBackgroundAction(QAction*)));

  connect(m_Recent, SIGNAL(actionTriggered(QAction*)), this, SLOT(close()));

}

void LabelSelectionButtonPopupMenu::SetModel(GlobalUIModel *model)
{
  m_Model = model;

  // Listen to changes in active label, active label set
  LatentITKEventNotifier::connect(
        model->GetGlobalState()->GetDrawingColorLabelModel(),
        IRISEvent(), this, SLOT(onModelUpdate(const EventBucket &)));

  LatentITKEventNotifier::connect(
        model->GetGlobalState()->GetDrawOverFilterModel(),
        IRISEvent(), this, SLOT(onModelUpdate(const EventBucket &)));

  // Configure the recents menu
  m_Recent->SetModel(model);

  this->UpdateMenu();

  // Update the timestamp
  m_LastUpdateTime.Modified();
}


void LabelSelectionButtonPopupMenu::UpdateMenu()
{
  m_SubForeground->clear();
  m_SubBackground->clear();
  ColorLabelTable *clt = m_Model->GetDriver()->GetColorLabelTable();

  DrawOverFilter dof[] = {
    DrawOverFilter(PAINT_OVER_ALL, 0),
    DrawOverFilter(PAINT_OVER_VISIBLE, 0) };

  for(int i = 0; i < 2; i++)
    {
    QIcon icon = CreateColorBoxIcon(16, 16, GetBrushForDrawOverFilter(dof[i], clt));
    QString name = GetTitleForDrawOverFilter(dof[i], clt);
    QAction *action = m_SubBackground->addAction(icon, name);
    action->setData(QVariant::fromValue(dof[i]));
    action->setCheckable(true);
    }
  m_SubBackground->addSeparator();

  typedef ColorLabelTable::ValidLabelMap ValidMap;
  const ColorLabelTable::ValidLabelMap &vmap = clt->GetValidLabels();
  for(ValidMap::const_iterator it = vmap.begin(); it != vmap.end(); ++it)
    {
    const ColorLabel &cl = it->second;
    QIcon icon = CreateColorBoxIcon(16, 16, GetBrushForColorLabel(cl));
    QString name = GetTitleForColorLabel(cl);

    QAction *action_fg = m_SubForeground->addAction(icon, name);
    action_fg->setData(it->first);
    action_fg->setCheckable(true);

    QAction *action_bg = m_SubBackground->addAction(icon, name);
    action_bg->setData(
          QVariant::fromValue(DrawOverFilter(PAINT_OVER_ONE, it->first)));
    action_bg->setCheckable(true);
    }
}

void LabelSelectionButtonPopupMenu::UpdateCurrents()
{
  ColorLabelTable *clt = m_Model->GetDriver()->GetColorLabelTable();
  LabelType fg = m_Model->GetGlobalState()->GetDrawingColorLabel();
  DrawOverFilter bg = m_Model->GetGlobalState()->GetDrawOverFilter();

  // Go through and check/uncheck all actions
  QList<QAction *> afg = m_SubForeground->actions();
  for(int i = 0; i < afg.size(); i++)
    {
    QAction *action = afg.at(i);
    int label = action->data().toInt();
    const ColorLabel &cl = clt->GetColorLabel(label);

    // Check if the color label has updated
    if(cl.GetTimeStamp() > m_LastUpdateTime)
      {
      action->setIcon(CreateColorBoxIcon(16, 16, GetBrushForColorLabel(cl)));
      action->setText(GetTitleForColorLabel(cl));
      }

    if(label == fg)
      {
      action->setChecked(true);
      m_SubForeground->setIcon(action->icon());
      }
    else
      {
      action->setChecked(false);
      }
    }

  // Go through and check/uncheck all actions
  QList<QAction *> abg = m_SubBackground->actions();
  for(int i = 0; i < abg.size(); i++)
    {
    QAction *action = abg.at(i);
    if(action->isSeparator())
      continue;
    DrawOverFilter dfi = qvariant_cast<DrawOverFilter>(action->data());

    if(dfi.CoverageMode == PAINT_OVER_ONE)
      {
      const ColorLabel &cl = clt->GetColorLabel(dfi.DrawOverLabel);

      // Check if the color label has updated
      if(cl.GetTimeStamp() > m_LastUpdateTime)
        {
        action->setIcon(CreateColorBoxIcon(16, 16, GetBrushForColorLabel(cl)));
        action->setText(GetTitleForColorLabel(cl));
        }
      }

    if(dfi == bg)
      {
      action->setChecked(true);
      m_SubBackground->setIcon(action->icon());
      }
    else
      {
      action->setChecked(false);
      }
    }

  m_LastUpdateTime.Modified();
}

void LabelSelectionButtonPopupMenu::onModelUpdate(const EventBucket &bucket)
{
  if(bucket.HasEvent(DomainChangedEvent()))
    {
    this->UpdateMenu();
    this->UpdateCurrents();
    }
  else if(bucket.HasEvent(ValueChangedEvent()) || bucket.HasEvent(DomainDescriptionChangedEvent()))
    {
    this->UpdateCurrents();
    }
}

void LabelSelectionButtonPopupMenu::onForegroundAction(QAction *action)
{
  int label = action->data().toInt();
  m_Model->GetGlobalState()->SetDrawingColorLabel(label);
}

void LabelSelectionButtonPopupMenu::onBackgroundAction(QAction *action)
{
  DrawOverFilter dof = qvariant_cast<DrawOverFilter>(action->data());
  m_Model->GetGlobalState()->SetDrawOverFilter(dof);
}

