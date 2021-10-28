#include "ColorMapInteractionDelegate.h"
#include "ColorMapRenderer.h"
#include "ColorMapModel.h"
#include "ColorMapInspector.h"
#include <QPalette>
#include <QWidget>

void ColorMapInteractionDelegate::mousePressEvent(QMouseEvent *)
{
  m_Model->ProcessMousePressEvent(m_XSpace);
}

void ColorMapInteractionDelegate::mouseReleaseEvent(QMouseEvent *)
{
  m_Model->ProcessMouseReleaseEvent(m_XSpace);
}

void ColorMapInteractionDelegate::mouseMoveEvent(QMouseEvent *)
{
  m_Model->ProcessMouseDragEvent(m_XSpace);
}

void ColorMapInteractionDelegate::mouseDoubleClickEvent(QMouseEvent *)
{
  m_InspectorWidget->PromptUserForColor();
}

ColorMapInteractionDelegate::ColorMapInteractionDelegate(QWidget *parent)
  : QtInteractionDelegateWidget(parent)
{
  m_Model = NULL;
}


