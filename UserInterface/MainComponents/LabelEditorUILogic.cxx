/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: LabelEditorUILogic.cxx,v $
  Language:  C++
  Date:      $Date: 2010/07/01 21:40:24 $
  Version:   $Revision: 1.10 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#include "LabelEditorUILogic.h"
#include "UserInterfaceLogic.h"
#include "GlobalState.h"
#include "IRISException.h"
#include "IRISApplication.h"
#include "IRISImageData.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <FL/fl_ask.H>

LabelEditorUILogic
::LabelEditorUILogic()
{
  m_Parent = NULL;
  m_Driver = NULL;
  m_GlobalState = NULL;
}

LabelEditorUILogic
::~LabelEditorUILogic()
{
}

void
LabelEditorUILogic
::Register(UserInterfaceBase *parent)
{
  m_Parent = parent;
  m_Driver = parent->GetDriver();
  m_GlobalState = m_Driver->GetGlobalState();
}

const ColorLabel &
LabelEditorUILogic
::GetColorLabel(size_t iLabel)
{ 
  return m_Driver->GetColorLabelTable()->GetColorLabel(iLabel); 
}

void
LabelEditorUILogic
::SetColorLabel(size_t iLabel, const ColorLabel &xLabel)
{ 
  m_Driver->GetColorLabelTable()->SetColorLabel(iLabel, xLabel); 
}

void 
LabelEditorUILogic
::DisplayWindow()
{
  m_WinMain->show();  
}

void 
LabelEditorUILogic
::OnCloseAction()
{
  m_WinMain->hide();
}

void 
LabelEditorUILogic
::OnLabelListUpdate(unsigned int iCurrentLabel)
{
  // Clear the list of labels
  m_BrsLabelList->clear();

  // We will also find the first valid label
  size_t iFirstValidLabel = 0;

  // Get the list of labels from the system and populate the browser
  for(size_t i = 1; i < MAX_COLOR_LABELS; i++)
    {
    ColorLabel cl = GetColorLabel(i);
    if(cl.IsValid())
      {
      // Create a label string
      std::ostringstream oss;
      oss << std::setw(3) << i << ": " << cl.GetLabel();
      
      // Add to the list the label and and the id (as a special data)
      m_BrsLabelList->add(oss.str().c_str(), (void *)(i));

      // Set the first valid label
      if(iFirstValidLabel == 0) iFirstValidLabel = i;
      }
    }

  // Set the current label's display, or if 0 is passed in initialize 
  // at the very first label
  SetEditorLabel(iCurrentLabel > 0 ? iCurrentLabel : iFirstValidLabel);
}

void
LabelEditorUILogic
::SetEditorLabel(unsigned int iLabel)
{
  // The label must be valid!
  assert(GetColorLabel(iLabel).IsValid());
  
  // Find the label in the browser, and select it
  for(size_t i = 1; i <= static_cast<size_t>(m_BrsLabelList->size()); i++)
    {
    size_t id = (size_t) m_BrsLabelList->data(i);
    if(id == iLabel) 
      {
      m_BrsLabelList->select(i);
      break;
      }
    }
  
  // Get the label object
  ColorLabel cl = GetColorLabel(iLabel);

  // Fill out the form
  m_InLabelName->value(cl.GetLabel());
  m_InLabelOpacity->value(cl.GetAlpha() / 255.0 );
  m_GrpLabelColor->rgb( cl.GetRGB(0)/255.0, cl.GetRGB(1)/255.0, cl.GetRGB(2)/255.0 );
  m_InLabelId->value(iLabel);
  m_ChkVisibility->value(!cl.IsVisible());
  m_ChkMeshVisibility->value(!cl.IsVisibleIn3D());
}

int 
LabelEditorUILogic
::FindSpaceForNewLabel()
{
  // Find a place to stick in the new label
  size_t iLabel = 0;
  for(size_t i = 1; i < MAX_COLOR_LABELS; i++)
    if(!GetColorLabel(i).IsValid())
      { iLabel = i; break; }

  // Make sure we actually found a space
  if(iLabel == 0)
    {
    fl_alert(
      "It is not possible to define more than %d labels in SNAP. \n"
      "Delete some of the existing labels to make room for the \n"
      "label that you wish to add. \n", MAX_COLOR_LABELS);
    return 0;
    }

  if(m_ChkPromptId->value())
    {
    // Show the prompt asking for the label Id
    std::ostringstream oss; oss << iLabel; 
    std::string sMessage = "";
    bool flagChosen = false;

    while(!flagChosen)
      {
      const char *sAnswer = 
        fl_input("What id would you like to assign to this label?",oss.str().c_str());

      // What if they click cancel?
      if(!sAnswer)
        return 0;

      try {
        int iAnswer = atoi(sAnswer);
        if(iAnswer <= 0 || iAnswer > MAX_COLOR_LABELS)
          fl_message("ERROR: You must enter a number between 1 and %d!\n", MAX_COLOR_LABELS);
        else if(GetColorLabel(iAnswer).IsValid())
          fl_message("ERROR: That label is already in use!\n");
        else
          { iLabel = iAnswer; flagChosen = true; }
      } 
      catch(...)
        { 
        fl_message("ERROR: You must enter a number between 1 and %d!\n", MAX_COLOR_LABELS);
        }
      }
    }

  // If the space is not the last one in the list, alert the user that we 
  // are sticking the label in the first available slot
  else if(iLabel < static_cast<size_t>(m_BrsLabelList->size()))
    {
    fl_message(
      "The label will be assigned the first available ID, which is %d\n"
      "To assign a different ID to the label or to move it to the \n"
      "bottom of the list of labels, use the 'Label Id' tab \n", static_cast<int>(iLabel));
    }
 
  return iLabel;
}

void
LabelEditorUILogic
::OnNewAction()
{
  // Find the space for this new label
  unsigned int iLabel = FindSpaceForNewLabel();
  if(iLabel == 0) return;
  
  // Get the default color label for this position
  ColorLabel cl = m_Driver->GetColorLabelTable()->GetDefaultColorLabel(iLabel);
  cl.SetValid(true);

  // Set the label
  SetColorLabel(iLabel, cl);

  // Update the state of the widget
  OnLabelListUpdate(iLabel);

  // Update the list in the parent UI
  m_Parent->OnLabelListUpdate();
}

void 
LabelEditorUILogic
::OnDuplicateAction()
{
  // Only valid if a label is selected
  assert(m_BrsLabelList->value() > 0);
  
  // Find the space for this new label
  unsigned int iNewLabel = FindSpaceForNewLabel();
  if(iNewLabel == 0) return;

  // Get the copy label (currently selected)
  size_t iCurrentLabel = (size_t) m_BrsLabelList->data(m_BrsLabelList->value());
 
  // Get the old and the new label 
  ColorLabel clNew = GetColorLabel(iNewLabel);
  const ColorLabel &clOld = GetColorLabel(iCurrentLabel);
  
  // Copy the label properties
  clNew.SetValid(true);
  clNew.SetPropertiesFromColorLabel(clOld);
  
  // Set the description
  std::ostringstream oss;
  oss << clOld.GetLabel() << "(Copy)";
  clNew.SetLabel(oss.str().c_str());

  // Store the label
  SetColorLabel(iNewLabel, clNew);

  // Update the state of the widget
  OnLabelListUpdate(iNewLabel);

  // Update the list in the parent UI
  m_Parent->OnLabelListUpdate();
}

size_t
LabelEditorUILogic
::GetSelectedLabelId()
{
  if(m_BrsLabelList->value() == 0) return 0;
  return (size_t) m_BrsLabelList->data(m_BrsLabelList->value());
}

void 
LabelEditorUILogic
::OnDeleteAction()
{
  // Must have a current label
  size_t iCurrentLabel = GetSelectedLabelId();
  assert(iCurrentLabel > 0);

  // Find the next label for us to activate
  size_t iNextLabel = 0;
  for(size_t i = iCurrentLabel + 1; i < MAX_COLOR_LABELS; i++)
    if(GetColorLabel(i).IsValid())
      { iNextLabel = i; break; }

  if(iNextLabel == 0)
    {
    // Turns out we are deleting the last label. Search back to the first label
    for(size_t j = iCurrentLabel - 1; j > 0; j--)
      if(GetColorLabel(j).IsValid())
        { iNextLabel = j; break; }
    }

  if(iNextLabel == 0)
    {
    // We're removing the only label. We can't allow that!
    fl_alert(
      "SNAP can not operate without any labels. Please add \n"
      "some new labels before deleting this one.");
    return;
    }
     
  // Delete the label in the system, replacing it with zero
  size_t nUpdated = m_Driver->GetCurrentImageData()->GetSegmentation()
    ->ReplaceIntensity(iCurrentLabel, 0);

  // Set the label current label as invalid
  m_Driver->GetColorLabelTable()->SetColorLabelValid(iCurrentLabel, false);

  // If the label is a current paint-over label or a drawing label, 
  // reposition the currently selected label
  if(iCurrentLabel == m_GlobalState->GetDrawingColorLabel())
    m_GlobalState->SetDrawingColorLabel(
      m_Driver->GetColorLabelTable()->GetFirstValidLabel());
  if(iCurrentLabel == m_GlobalState->GetOverWriteColorLabel())
    m_GlobalState->SetOverWriteColorLabel(0);
  
  // Rebuild the list of labels
  OnLabelListUpdate(iNextLabel);

  // The segmentation image has been modified, let the parent know
  if(nUpdated > 0)
    {
    // This operation can not be undone!
    m_Parent->ClearUndoPoints();
    m_Parent->OnSegmentationImageUpdate(false);    
    }
    
  // Update the label list
  m_Parent->OnLabelListUpdate();

}

void
LabelEditorUILogic
::OnDeleteAllAction()
{
  // Reset the label table to the defaults (six labels)
  m_Driver->GetColorLabelTable()->InitializeToDefaults();

  // We also must make sure that all labels in the segmentation image are there
  if(m_Driver->GetIRISImageData()->IsSegmentationLoaded())
    {
    LabelImageWrapper::ConstIterator it = 
      m_Driver->GetIRISImageData()->GetSegmentation()->GetImageConstIterator();
    for( ; !it.IsAtEnd(); ++it)
      if(!m_Driver->GetColorLabelTable()->IsColorLabelValid(it.Get()))
        m_Driver->GetColorLabelTable()->SetColorLabelValid(it.Get(), true);
    }

  // Reset the drawing and draw-over
  m_GlobalState->SetDrawingColorLabel(
    m_Driver->GetColorLabelTable()->GetFirstValidLabel());
  m_GlobalState->SetOverWriteColorLabel(0);

  // Update the UI
  this->OnLabelListUpdate(m_GlobalState->GetDrawingColorLabel());
  m_Parent->OnLabelListUpdate();
}
  
void
LabelEditorUILogic
::OnSetIdAction()
{
  // Must have a current label
  size_t iCurrentLabel = GetSelectedLabelId();
  assert(iCurrentLabel > 0);

  // Get the id and check that it's valid
  unsigned int iNewId = (unsigned int) m_InLabelId->value();
  if(iNewId <= 0 || iNewId > MAX_COLOR_LABELS)
    {
    fl_alert("The label id must be a number between 1 and %d", MAX_COLOR_LABELS);
    return;
    }

  // Check if the id is already in use
  ColorLabel clTarget = GetColorLabel(iNewId);
  if(clTarget.IsValid())
    {
    fl_alert("Label %d is already in use! Please select a different id", iNewId);
    return;
    }

  // Move the label into the new slot
  clTarget.SetPropertiesFromColorLabel(GetColorLabel(iCurrentLabel));
  clTarget.SetValid(true);
  SetColorLabel(iNewId, clTarget);

  // Update the new labels
  m_Driver->GetColorLabelTable()->SetColorLabelValid(iCurrentLabel, false);

  // If the label was a current drawing label, we need to update that
  if(iCurrentLabel == m_GlobalState->GetDrawingColorLabel())
    m_GlobalState->SetDrawingColorLabel(iNewId);
  if(iCurrentLabel == m_GlobalState->GetOverWriteColorLabel())
    m_GlobalState->SetOverWriteColorLabel(iNewId);

  // Replace label iOld with iNew in the segmentation image
  size_t nUpdated = m_Driver->GetCurrentImageData()->GetSegmentation()
    ->ReplaceIntensity(iCurrentLabel, iNewId);

  // Rebuild the list of labels
  OnLabelListUpdate(iNewId);

  // The segmentation image has been modified, let the parent know
  if(nUpdated > 0)
    {
    // This operation can not be undone!
    m_Parent->ClearUndoPoints();
    m_Parent->OnSegmentationImageUpdate(false);    
    }
}
  
void
LabelEditorUILogic
::OnLabelPropertyChange()
{
  // Make sure there is a selected label
  size_t iCurrentLabel = GetSelectedLabelId();
  assert(iCurrentLabel > 0);

  // Get the label
  ColorLabel cl = GetColorLabel(iCurrentLabel);

  // Get the color values
  unsigned char rgba[4];
  rgba[0] = (unsigned char) (255 * m_GrpLabelColor->r());
  rgba[1] = (unsigned char) (255 * m_GrpLabelColor->g());
  rgba[2] = (unsigned char) (255 * m_GrpLabelColor->b());
  rgba[3] = (unsigned char) (255 * m_InLabelOpacity->value());

  // Set the values of the label
  cl.SetLabel(m_InLabelName->value());
  cl.SetRGBAVector(rgba); 

  // If visibility has changed, we need to enable mesh update
  bool old_visible = cl.IsVisible(), old_vis3d = cl.IsVisibleIn3D();
  cl.SetVisible(m_ChkVisibility->value() == 0);
  cl.SetVisibleIn3D(m_ChkMeshVisibility->value() == 0);
  if((!old_vis3d && cl.IsVisibleIn3D()) || (!old_visible && cl.IsVisible()))
    {
    m_Parent->OnIRISMeshDisplaySettingsUpdate();
    }

  // Store the new label
  SetColorLabel(iCurrentLabel, cl);

  // Update the label list
  // OnLabelListUpdate(iCurrentLabel);
 
  // Update the text in the browser
  std::ostringstream oss;
  oss << std::setw(3) << iCurrentLabel << ": " << cl.GetLabel();
  m_BrsLabelList->text(m_BrsLabelList->value(), oss.str().c_str());

  // Update the list in the parent UI
  m_Parent->OnLabelListUpdate();
}
  
void
LabelEditorUILogic
::OnMoveUpAction()
{
}

void
LabelEditorUILogic
::OnMoveDownAction()
{
}
  
void
LabelEditorUILogic
::OnLabelSelectAction()
{
  SetEditorLabel(GetSelectedLabelId());
}

const size_t 
LabelEditorUILogic
::TOOL_PAGE_LABEL_MENU_COUNT = 5;

void
LabelEditorUILogic
::OnToolsDialogAction()
{
  // Flags decribing whether each dropdown includes the clear label
  int flagIncludeClear[TOOL_PAGE_LABEL_MENU_COUNT] = { 0, 1, 1, 0, 0 };
  int flagInitToCurrent[TOOL_PAGE_LABEL_MENU_COUNT] = { 0, 1, 1, 0, 1 };
  int flagInitToDrawOver[TOOL_PAGE_LABEL_MENU_COUNT] = { 1, 0, 0, 1, 0 };

  // Initialize the menus
  m_ToolPageLabelMenu = new Fl_Menu_Item*[TOOL_PAGE_LABEL_MENU_COUNT];

  // Populate the label dropdowns
  for(size_t i = 0; i < TOOL_PAGE_LABEL_MENU_COUNT; i++)
    {
    // Populate the dropdown
    m_ToolPageLabelMenu[i] = m_Parent->GenerateColorLabelMenu(
      false, false, flagIncludeClear[i]);
    m_InToolPageLabel[i]->menu(m_ToolPageLabelMenu[i]);

    // Find an item to select
    LabelType selected = this->GetSelectedLabelId();
    LabelType drawover = m_Driver->GetGlobalState()->GetOverWriteColorLabel();
    for(Fl_Menu_Item *p = m_ToolPageLabelMenu[i]; p->text; ++p)
      {
      LabelType id = (LabelType)(size_t)p->user_data();
      if(flagInitToCurrent[i] && id == selected)
        m_InToolPageLabel[i]->value(p);
      else if(flagInitToDrawOver[i] && id == drawover)
        m_InToolPageLabel[i]->value(p);
      }
    }  
  
  // Show the dialog
  UserInterfaceLogic::CenterChildWindowInParentWindow(m_WinTools, m_WinMain);
  m_WinTools->show();
}

#include "itkTopologyPreservingDigitalSurfaceEvolutionImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"

void
LabelEditorUILogic
::OnToolsApplyAction()
{
  // Relabeling operation
  if(m_InToolsOperation->value() == 0)
    {
    // Get the labels
    LabelType src = (LabelType) (size_t) m_InToolPageLabel[0]->mvalue()->user_data();
    LabelType trg = (LabelType) (size_t) m_InToolPageLabel[1]->mvalue()->user_data();

    // Do the simple merge
    size_t nChanged = m_Driver->ReplaceLabel(trg, src);
    if(nChanged == 0)
      fl_alert("No voxels were affected by this operation!");
    else
      {
      m_Parent->StoreUndoPoint("Relabeling");
      m_Parent->OnSegmentationImageUpdate(false);
      }
    }

  // Topological merge operation
  else if(m_InToolsOperation->value() == 1)
    {
    // Get the segmentation image
    typedef itk::Image<LabelType, 3> LabelImageType;
    LabelImageType::Pointer iSeg =
      m_Driver->GetCurrentImageData()->GetSegmentation()->GetImage();

    // Get the labels
    LabelType l_src = (LabelType) (size_t) m_InToolPageLabel[0]->mvalue()->user_data();
    LabelType l_trg = (LabelType) (size_t) m_InToolPageLabel[1]->mvalue()->user_data();

    // Extract binary images for each label
    typedef itk::BinaryThresholdImageFilter<LabelImageType,LabelImageType> ThreshType;
    ThreshType::Pointer tsrc = ThreshType::New();
    tsrc->SetInput(iSeg);
    tsrc->SetLowerThreshold(l_src);
    tsrc->SetUpperThreshold(l_src);
    tsrc->SetInsideValue(1);
    tsrc->SetOutsideValue(0);
    tsrc->Update();
    
    ThreshType::Pointer ttrg = ThreshType::New();
    ttrg->SetInput(iSeg);
    ttrg->SetLowerThreshold(l_trg);
    ttrg->SetUpperThreshold(l_trg);
    ttrg->SetInsideValue(1);
    ttrg->SetOutsideValue(0);
    ttrg->Update();

    // Create a progress object
    typedef itk::MemberCommand<UserInterfaceBase> CommandType;
    CommandType::Pointer pcom = CommandType::New();
    pcom->SetCallbackFunction(m_Parent, &UserInterfaceBase::OnITKProgressEvent);

    // Create the evolution filter
    typedef itk::TopologyPreservingDigitalSurfaceEvolutionImageFilter<LabelImageType> FilterType;
    FilterType::Pointer filter = FilterType::New();
    filter->SetInput(tsrc->GetOutput());
    filter->SetTargetImage(ttrg->GetOutput());
    filter->SetNumberOfIterations(100);
    filter->SetForegroundValue(1);
    filter->SetUseInversionMode(false);
    filter->InPlaceOn();
    filter->AddObserver(itk::AnyEvent(), pcom);
    filter->Update();

    // Merge back into the segmentation
    typedef itk::ImageRegionIterator<LabelImageType> IteratorType;
    IteratorType itseg(iSeg, iSeg->GetBufferedRegion());
    IteratorType itnew(filter->GetOutput(), iSeg->GetBufferedRegion());
    size_t nChanged = 0;
    while(!itseg.IsAtEnd())
      {
      if(itnew.Get())
        {
        itseg.Set(l_src);
        ++nChanged;
        }
      ++itseg; ++itnew;
      }

    // Let parent know something happened
    if(nChanged)
      {
      iSeg->Modified();
      m_Parent->StoreUndoPoint("Topological Merge");
      m_Parent->OnSegmentationImageUpdate(false);
      }
    else
      {
      fl_alert("No voxel labels were changed");
      }
    }

  // Topological merge operation
  else if(m_InToolsOperation->value() == 2)
    {
    fl_alert("This operation is not yet implemented");
    }
}

void
LabelEditorUILogic
::OnToolsOperationChange()
{
  m_WizToolControls->value(
    m_WizToolControls->child(m_InToolsOperation->value()));
}

void
LabelEditorUILogic
::OnToolsCloseAction()
{
  // Empty all menus
  for(size_t i = 0; i < TOOL_PAGE_LABEL_MENU_COUNT; i++)
    {
    // Populate the dropdown
    m_InToolPageLabel[i]->clear();
    m_Parent->DeleteColorLabelMenu(m_ToolPageLabelMenu[i]);
    m_ToolPageLabelMenu[i] = NULL;
    }
  delete m_ToolPageLabelMenu;
  m_WinTools->hide();
}

