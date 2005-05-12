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
#include "vtkObjectFactory.h"

#include "vtkPVApplication.h"
#include "vtkPVAnimationManager.h"
#include "vtkKWLabel.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenu.h"
#include "vtkPVSource.h"
#include "vtkPVAnimationCueTree.h"
#include "vtkSmartPointer.h"
#include "vtkCollectionIterator.h"
#include "vtkPVTraceHelper.h"

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
vtkCxxRevisionMacro(vtkPVActiveTrackSelector, "1.1");
//-----------------------------------------------------------------------------
vtkPVActiveTrackSelector::vtkPVActiveTrackSelector()
{
  this->AnimationManager = 0;
  
  this->SourceLabel = vtkKWLabel::New();
  this->SourceMenuButton = vtkKWMenuButton::New();
  this->PropertyLabel = vtkKWLabel::New();
  this->PropertyMenuButton = vtkKWMenuButton::New();
  this->Internals = new vtkPVActiveTrackSelectorInternals;
  this->CurrentSourceCueTree = 0;
  
}
//-----------------------------------------------------------------------------
vtkPVActiveTrackSelector::~vtkPVActiveTrackSelector()
{
  this->CurrentSourceCueTree = 0;
  this->SetAnimationManager(0);
  this->SourceLabel->Delete();
  this->SourceMenuButton->Delete();
  this->PropertyLabel->Delete();
  this->PropertyMenuButton->Delete();
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::Create(vtkKWApplication* app, const char* args)
{
  if (!this->AnimationManager)
    {
    vtkErrorMacro("AnimationManager must be set before calling Create");
    return;
    }
  if (!this->Superclass::Create(app, "frame", args))
    {
    vtkErrorMacro("Failed creating the widget");
    return;
    }
  
  this->SourceLabel->SetParent(this);
  this->SourceLabel->SetText("Source");
  this->SourceLabel->Create(app, 0);
  
  this->SourceMenuButton->SetParent(this);
  this->SourceMenuButton->Create(app, 0);
  this->SourceMenuButton->SetBalloonHelpString("Select a Source to animate.");
  

  this->PropertyLabel->SetParent(this);
  this->PropertyLabel->SetText("Property");
  this->PropertyLabel->Create(app, 0);

  this->PropertyMenuButton->SetParent(this);
  this->PropertyMenuButton->Create(app, 0);
  this->PropertyMenuButton->SetBalloonHelpString(
    "Select a Property to animate for the choosen Source.");

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

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::SelectCue(vtkPVAnimationCue* cue)
{
  if (!cue)
    {
    this->CleanupSource();
    return;
    }
  
  const char* key = cue->GetPVSource()->GetName();
  this->SelectSourceCallbackInternal(key);
  
  vtkPVActiveTrackSelectorInternals::VectorOfCues::iterator iter =
    this->Internals->PropertyCues.begin();
  int index = 0;
  for (; iter != this->Internals->PropertyCues.end(); ++iter, ++index)
    {
    if (iter->GetPointer() == cue)
      {
      this->SelectPropertyCallbackInternal(index);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::AddSource(vtkPVAnimationCueTree* cue)
{
  if (!cue)
    {
    return;
    }
 
  const char* key = cue->GetPVSource()->GetName();
  this->Internals->SourceCueTrees[key] = cue;;
  
  ostrstream command;
  command << "SelectSourceCallback " << key  << ends;
  this->SourceMenuButton->AddCommand(cue->GetLabelText(),
    this, command.str(), "Select Source");
  command.rdbuf()->freeze(0);
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::RemoveSource(vtkPVAnimationCueTree* cue)
{
  if (!cue)
    {
    return;
    }

  const char*key = cue->GetPVSource()->GetName();
  
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
  this->SourceMenuButton->GetMenu()->DeleteMenuItem(cue->GetLabelText());
  this->Internals->SourceCueTrees.erase(iter);
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::CleanupSource()
{
  this->CleanupPropertiesMenu();
  this->CurrentSourceCueTree = 0;
  this->SourceMenuButton->SetButtonText(""); 
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::SelectSourceCallback(const char* key)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) SelectSourceCallback %s",
    this->GetTclName(), key);
  this->SelectSourceCallbackInternal(key);
  if (this->CurrentSourceCueTree)
    {
    this->CurrentSourceCueTree->GetFocus();
    }
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
  this->SourceMenuButton->SetButtonText(cueTree->GetLabelText());
  this->BuildPropertiesMenu(0, cueTree);
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::CleanupPropertiesMenu()
{
  this->PropertyMenuButton->GetMenu()->DeleteAllMenuItems();
  this->Internals->PropertyCues.clear();
  this->PropertyMenuButton->SetButtonText("");
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
      this->PropertyMenuButton->AddCommand(label.str(), this,
        command.str(), "Select Property");
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
  cue->GetFocus();
}
  

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::SelectPropertyCallbackInternal(int cue_index)
{
  this->PropertyMenuButton->SetButtonText(
    this->PropertyMenuButton->GetMenu()->GetItemLabel(cue_index));
}

//-----------------------------------------------------------------------------
void vtkPVActiveTrackSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
