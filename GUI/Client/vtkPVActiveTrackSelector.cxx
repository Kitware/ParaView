/*=========================================================================

  Program:   ParaView
  Module:    vtkPVActiveTrackSelector.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVActiveTrackSelector.h"

#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationCueTree.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>
#include <vtkstd/map>
#include <vtkstd/string>

class vtkPVActiveTrackSelectorInternals
{
public:
  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPVAnimationCueTree> > MapOfStringToCueTrees;
  typedef vtkstd::vector<vtkSmartPointer<vtkPVAnimationCue> > VectorOfCues;
  MapOfStringToCueTrees SourceCueTrees;
  VectorOfCues PropertyCues;
};

vtkStandardNewMacro(vtkPVActiveTrackSelector);
vtkCxxRevisionMacro(vtkPVActiveTrackSelector, "1.15");
//-----------------------------------------------------------------------------
vtkPVActiveTrackSelector::vtkPVActiveTrackSelector()
{
  this->SourceLabel = vtkKWLabel::New();
  this->SourceMenuButton = vtkKWMenuButton::New();
  this->PropertyLabel = vtkKWLabel::New();
  this->PropertyMenuButton = vtkKWMenuButton::New();
  this->Internals = new vtkPVActiveTrackSelectorInternals;
  this->CurrentSourceCueTree = 0;
  this->CurrentCue = 0;
  this->PackHorizontally = 0;
  this->FocusCurrentCue = 1;
}
//-----------------------------------------------------------------------------
vtkPVActiveTrackSelector::~vtkPVActiveTrackSelector()
{
  this->CurrentSourceCueTree = 0;
  this->SourceLabel->Delete();
  this->SourceMenuButton->Delete();
  this->PropertyLabel->Delete();
  this->PropertyMenuButton->Delete();
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();
  
  this->SourceLabel->SetParent(this);
  this->SourceLabel->SetText("Source:");
  this->SourceLabel->Create();
  
  this->SourceMenuButton->SetParent(this);
  this->SourceMenuButton->Create();
  this->SourceMenuButton->SetBalloonHelpString("Select a Source to animate.");
  this->SourceMenuButton->SetValue("Unselected"); 
  
  this->PropertyLabel->SetParent(this);
  this->PropertyLabel->SetText("Property:");
  this->PropertyLabel->Create();

  this->PropertyMenuButton->SetParent(this);
  this->PropertyMenuButton->Create();
  this->PropertyMenuButton->SetBalloonHelpString(
    "Select a Property to animate for the choosen Source.");
  this->PropertyMenuButton->SetValue("Unselected"); 

  if (!this->PackHorizontally)
    {
    this->Script("grid %s %s -sticky news -padx 2 -pady 2",
                 this->SourceLabel->GetWidgetName(),
                 this->SourceMenuButton->GetWidgetName());
    this->Script("grid %s %s -sticky news -padx 2 -pady 2",
                 this->PropertyLabel->GetWidgetName(),
                 this->PropertyMenuButton->GetWidgetName());
    this->Script("grid columnconfigure %s 0 -weight 0 ",
                 this->GetWidgetName());
    this->Script("grid columnconfigure %s 1 -weight 2 ",
                 this->GetWidgetName());
    }
  else
    {
    this->Script("grid %s %s %s %s",
                 this->SourceLabel->GetWidgetName(),
                 this->SourceMenuButton->GetWidgetName(),
                 this->PropertyLabel->GetWidgetName(),
                 this->PropertyMenuButton->GetWidgetName());
    }
}

//-----------------------------------------------------------------------------
int vtkPVActiveTrackSelector::SelectCue(const char* sourceName, 
                                        vtkSMAnimationCueProxy* cue)
{
  if (!cue)
    {
    this->CleanupSource();
    return 1;
    }
  
  this->SelectSourceCallbackInternal(sourceName);

  vtkPVActiveTrackSelectorInternals::VectorOfCues::iterator iter =
    this->Internals->PropertyCues.begin();
  int index = 0;
  for (; iter != this->Internals->PropertyCues.end(); ++iter, ++index)
    {
    if (iter->GetPointer())
      {
      vtkSMAnimationCueProxy* proxy = iter->GetPointer()->GetCueProxy();
      if (proxy && proxy->GetAnimatedProxy() == cue->GetAnimatedProxy() &&
          strcmp(proxy->GetAnimatedPropertyName(), cue->GetAnimatedPropertyName()) == 0 &&
          proxy->GetAnimatedElement() == cue->GetAnimatedElement())
        {
        this->SelectPropertyCallbackInternal(index);
        return 1;
        }
      }
    }
  return 0;

}

//-----------------------------------------------------------------------------
int vtkPVActiveTrackSelector::SelectCue(vtkPVAnimationCue* cue)
{
  if (!cue)
    {
    this->CleanupSource();
    return 1;
    }
  
  const char* key = (cue->GetPVSource())? cue->GetPVSource()->GetName():
    cue->GetSourceTreeName();

  this->SelectSourceCallbackInternal(key);
  
  vtkPVActiveTrackSelectorInternals::VectorOfCues::iterator iter =
    this->Internals->PropertyCues.begin();
  int index = 0;
  for (; iter != this->Internals->PropertyCues.end(); ++iter, ++index)
    {
    if (iter->GetPointer() == cue)
      {
      this->SelectPropertyCallbackInternal(index);
      return 1;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::AddSource(vtkPVAnimationCueTree* cue)
{
  if (!cue)
    {
    return;
    }
 
  const char* key = (cue->GetPVSource())? cue->GetPVSource()->GetName() : 
    cue->GetName();
  this->Internals->SourceCueTrees[key] = cue;;
  
  ostrstream command;
  command << "SelectSourceCallback " << key  << ends;
  this->SourceMenuButton->GetMenu()->AddCommand(
    cue->GetLabelText(), this, command.str());
  command.rdbuf()->freeze(0);
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::RemoveSource(vtkPVSource* source)
{
  vtkPVActiveTrackSelectorInternals::MapOfStringToCueTrees::iterator iter =
    this->Internals->SourceCueTrees.begin();
  for (; iter != this->Internals->SourceCueTrees.end(); iter++)
    {
    if (source == iter->second->GetPVSource())
      {
      this->RemoveSource(iter->second);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::RemoveSource(vtkPVAnimationCueTree* cue)
{
  if (!cue)
    {
    return;
    }

  const char*key = (cue->GetPVSource())? cue->GetPVSource()->GetName() : 
    cue->GetName();
  
  vtkPVActiveTrackSelectorInternals::MapOfStringToCueTrees::iterator iter =
    this->Internals->SourceCueTrees.find(key);
  if (iter == this->Internals->SourceCueTrees.end())
    {
    return;
    }
  
  // check if the cue removed was the currently selected cue.
  if (this->CurrentSourceCueTree == cue)
    {
    this->CleanupSource();
    }
  this->SourceMenuButton->GetMenu()->DeleteItem(
    this->SourceMenuButton->GetMenu()->GetIndexOfItem(cue->GetLabelText()));
  this->Internals->SourceCueTrees.erase(iter);
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::ShallowCopy(
  vtkPVActiveTrackSelector* source, int onlyCopySources)
{
  vtkPVActiveTrackSelectorInternals::MapOfStringToCueTrees::iterator iter =
    source->Internals->SourceCueTrees.begin();
  for (; iter != source->Internals->SourceCueTrees.end(); iter++)
    {
    if (!onlyCopySources || iter->second->GetPVSource())
      {
      this->AddSource(iter->second);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::CleanupSource()
{
  this->CleanupPropertiesMenu();
  this->CurrentSourceCueTree = 0;
  this->SourceMenuButton->SetValue("Unselected"); 
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::SelectSourceCallback(const char* key)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SelectSourceCallback %s",
    this->GetTclName(), key);
  this->SelectSourceCallbackInternal(key);
  if (this->CurrentSourceCueTree && this->FocusCurrentCue)
    {
    this->CurrentSourceCueTree->GetFocus();
    }
  this->CurrentCue = 0;
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::SelectSourceCallbackInternal(
  const char* key)
{
  
  vtkPVActiveTrackSelectorInternals::MapOfStringToCueTrees::iterator iter;
  if (key)
    {
    iter = this->Internals->SourceCueTrees.find(key);
    }
  
  if (!key || iter == this->Internals->SourceCueTrees.end())
    {
    this->CleanupSource();
    return ;
    }
  
  vtkPVAnimationCueTree* cueTree = iter->second.GetPointer();
  this->CurrentSourceCueTree = cueTree;
  this->SourceMenuButton->SetValue(cueTree->GetLabelText());
  this->BuildPropertiesMenu(0, cueTree);
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::CleanupPropertiesMenu()
{
  this->PropertyMenuButton->GetMenu()->DeleteAllItems();
  this->Internals->PropertyCues.clear();
  this->PropertyMenuButton->SetValue("Unselected");
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::BuildPropertiesMenu(const char* pretext,
  vtkPVAnimationCueTree* cueTree)
{
  if (pretext==0)
    {
    // clean up old stuff.
    this->CleanupPropertiesMenu();
    }
  vtkCollectionIterator* child_iter = cueTree->NewChildrenIterator();
  for (child_iter->InitTraversal();
    !child_iter->IsDoneWithTraversal(); child_iter->GoToNextItem())
    {
    vtkPVAnimationCueTree* child_tree = vtkPVAnimationCueTree::SafeDownCast(
      child_iter->GetCurrentObject());
    vtkPVAnimationCue* child_cue = vtkPVAnimationCue::SafeDownCast(
      child_iter->GetCurrentObject());
    ostrstream label;
    if (pretext)
      {
      label << pretext << " : ";
      }
    label << child_cue->GetLabelText() << ends;

    if (child_tree)
      {
      this->BuildPropertiesMenu(label.str(), child_tree);
      }
    else if (child_cue)
      {
      int index = this->Internals->PropertyCues.size();
      this->Internals->PropertyCues.push_back(child_cue);
      
      ostrstream command;
      command << "SelectPropertyCallback " << index << ends;
      this->PropertyMenuButton->GetMenu()->AddCommand(
        label.str(), this, command.str());
      command.rdbuf()->freeze(0);
      }
    label.rdbuf()->freeze(0);
    }
  child_iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::SelectPropertyCallback(int cue_index)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SelectPropertyCallback %d",
    this->GetTclName(), cue_index);

  this->SelectPropertyCallbackInternal(cue_index);
  vtkPVAnimationCue* cue= 
    this->Internals->PropertyCues[cue_index].GetPointer();
  if( this->FocusCurrentCue )
    {
    cue->GetFocus();
    }
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent);
}
  

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::SelectPropertyCallbackInternal(int cue_index)
{
  const char* selected_label = 
    this->PropertyMenuButton->GetMenu()->GetItemLabel(cue_index);
  if (selected_label)
    {
    char* temp = new char[strlen(selected_label) + 1];
    strcpy(temp, selected_label);
    this->PropertyMenuButton->SetValue(temp);
    delete [] temp;
    }
  else
    {
    this->PropertyMenuButton->SetValue("Unselected");
    }

  vtkPVAnimationCue* cue= 
    this->Internals->PropertyCues[cue_index].GetPointer();
  this->CurrentCue = cue;
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::UpdateEnableState()
{
  this->PropagateEnableState(this->SourceMenuButton);
  this->PropagateEnableState(this->PropertyMenuButton);
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PropertyMenuButton: ";
  if (this->PropertyMenuButton)
    {
    this->PropertyMenuButton->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "SourceMenuButton: ";
  if (this->SourceMenuButton)
    {
    this->SourceMenuButton->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "CurrentCue: ";
  if (this->CurrentCue)
    {
    this->CurrentCue->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "PackHorizontally: " << this->PackHorizontally << endl;
  os << indent << "FocusCurrentCue: " << this->FocusCurrentCue << endl;
}
