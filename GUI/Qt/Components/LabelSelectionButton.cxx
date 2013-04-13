#include "LabelSelectionButton.h"
#include "GlobalUIModel.h"
#include "AbstractModel.h"
#include "LatentITKEventNotifier.h"
#include "GlobalState.h"
#include "IRISApplication.h"
#include "ColorLabelTable.h"
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsDropShadowEffect>
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

  // TODO: this could be made a little prettier
  QGraphicsScene scene(0,0,22,22);

  QPixmap pm(22, 22);
  pm.fill(QColor(0,0,0,0));

  QPainter qp(&pm);

  LabelType fg = m_Model->GetGlobalState()->GetDrawingColorLabel();
  DrawOverFilter bg = m_Model->GetGlobalState()->GetDrawOverFilter();

  QBrush brush_fg = GetBrushForColorLabel(fg, clt);
  QBrush brush_bg = GetBrushForDrawOverFilter(bg, clt);

  QGraphicsItem *item_bg = scene.addRect(7,7,12,12,QPen(Qt::black), brush_bg);
  scene.addRect(2,2,12,12,QPen(Qt::black), brush_fg);

  QGraphicsDropShadowEffect *eff_bg = new QGraphicsDropShadowEffect(&scene);
  eff_bg->setBlurRadius(1.0);
  eff_bg->setOffset(1.0);
  eff_bg->setColor(QColor(63,63,63,100));
  item_bg->setGraphicsEffect(eff_bg);

  scene.render(&qp);

  // Draw a split button
  this->setIcon(QIcon(pm));

  // Update the tooltip
  QString tooltip = QString(
        "<html><head/><body>"
        "<p>Foreground label:<br><span style=\" font-weight:600;\">%1</span></p>"
        "<p>Background label:<br><span style=\" font-weight:600;\">%2</span></p>"
        "</body></html>").
      arg(GetTitleForColorLabel(fg, clt)).arg(GetTitleForDrawOverFilter(bg, clt));
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


LabelSelectionButtonPopupMenu::LabelSelectionButtonPopupMenu(QWidget *parent)
  : QMenu(parent)
{
  m_SubForeground = this->addMenu("Foreground Label");
  m_SubBackground = this->addMenu("Background Label");
  this->addSeparator();
  this->addAction(FindUpstreamAction(this, "actionLabel_Editor"));

  this->setStyleSheet("font-size: 12px;");

  connect(m_SubForeground, SIGNAL(triggered(QAction*)), SLOT(onForegroundAction(QAction*)));
  connect(m_SubBackground, SIGNAL(triggered(QAction*)), SLOT(onBackgroundAction(QAction*)));
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

