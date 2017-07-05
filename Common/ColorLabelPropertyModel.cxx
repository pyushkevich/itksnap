#include "ColorLabelPropertyModel.h"
#include <GlobalState.h>

void ConcreteColorLabelPropertyModel
::Initialize(ColorLabelTable *clt)
{
  // Ititialize the domain representation
  DomainType dom(&clt->GetValidLabels());
  this->SetDomain(dom);

  // We should also listen to events from the label table, and rebroadcast
  // as changes to the domain. Note that there are two types of changes to the
  // label table, one that is a reconfiguration and another that is a property
  // change. These map to different kinds of domain change events.
  Rebroadcast(clt, SegmentationLabelConfigurationChangeEvent(), DomainChangedEvent());
  Rebroadcast(clt, SegmentationLabelPropertyChangeEvent(), DomainDescriptionChangedEvent());
}

DrawOverLabelItemSetIterator
::DrawOverLabelItemSetIterator(ColorLabelTable *table)
{
  m_LabelIter = table->GetValidLabels().begin();
  m_PointedMode = PAINT_OVER_ALL;
}

DrawOverLabelItemSetIterator&
DrawOverLabelItemSetIterator
::operator ++()
{
  // Iterate over modes
  if(m_PointedMode == PAINT_OVER_ALL)
    m_PointedMode = PAINT_OVER_VISIBLE;
  else if(m_PointedMode == PAINT_OVER_VISIBLE)
    m_PointedMode = PAINT_OVER_ONE;
  else
    ++m_LabelIter;

  return *this;
}

bool
DrawOverLabelItemSetIterator
::operator ==(const Self &comp)
{
  return (m_PointedMode == comp.m_PointedMode)
      && (m_LabelIter == comp.m_LabelIter);
}

bool
DrawOverLabelItemSetIterator
::operator !=(const Self &comp)
{
  return (m_PointedMode != comp.m_PointedMode)
      || (m_LabelIter != comp.m_LabelIter);
}

DrawOverLabelItemSetDomain
::DrawOverLabelItemSetDomain()
{
  m_LabelTable = NULL;
}

void DrawOverLabelItemSetDomain
::Initialize(ColorLabelTable *clt)
{
  this->m_LabelTable = clt;
}

DrawOverLabelItemSetDomain::const_iterator
DrawOverLabelItemSetDomain::begin() const
{
  const_iterator it(m_LabelTable);
  return it;
}

DrawOverLabelItemSetDomain::const_iterator
DrawOverLabelItemSetDomain::end() const
{
  const_iterator it(m_LabelTable);
  it.m_PointedMode = PAINT_OVER_ONE;
  it.m_LabelIter = m_LabelTable->end();
  return it;
}

DrawOverLabelItemSetDomain::const_iterator
DrawOverLabelItemSetDomain::find(const DrawOverFilter &value) const
{
  const_iterator it(m_LabelTable);
  it.m_PointedMode = value.CoverageMode;
  if(value.CoverageMode == PAINT_OVER_ONE)
    it.m_LabelIter = m_LabelTable->GetValidLabels().find(value.DrawOverLabel);
  return it;
}

DrawOverFilter
DrawOverLabelItemSetDomain::GetValue(const const_iterator &it) const
{
  DrawOverFilter filter;
  filter.CoverageMode = it.m_PointedMode;
  filter.DrawOverLabel = it.m_LabelIter->first;
  return filter;
}

ColorLabel
DrawOverLabelItemSetDomain::GetDescription(const const_iterator &it) const
{
  // For the override modes, return a dummy color label
  if(it.m_PointedMode != PAINT_OVER_ONE)
    return ColorLabel();
  else
    return it.m_LabelIter->second;
}

bool
DrawOverLabelItemSetDomain::operator == (
    const DrawOverLabelItemSetDomain &refdom)
{
  return m_LabelTable == refdom.m_LabelTable;
}

bool
DrawOverLabelItemSetDomain::operator != (
    const DrawOverLabelItemSetDomain &refdom)
{
  return m_LabelTable != refdom.m_LabelTable;
}


void ConcreteDrawOverFilterPropertyModel
::Initialize(ColorLabelTable *clt)
{
  // Ititialize the domain representation
  DomainType dom;
  dom.Initialize(clt);
  this->SetDomain(dom);

  // We should also listen to events from the label table, and rebroadcast
  // as changes to the domain. Note that there are two types of changes to the
  // label table, one that is a reconfiguration and another that is a property
  // change. These map to different kinds of domain change events.
  Rebroadcast(clt, SegmentationLabelConfigurationChangeEvent(), DomainChangedEvent());
  Rebroadcast(clt, SegmentationLabelPropertyChangeEvent(), DomainDescriptionChangedEvent());
}
