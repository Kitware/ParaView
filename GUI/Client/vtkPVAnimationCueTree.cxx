/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationCueTree.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimationCueTree.h"

#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWCanvas.h"
#include "vtkKWLabel.h"
#include "vtkCollectionIterator.h"
#include "vtkCollection.h"
#include "vtkPVTimeLine.h"
#include "vtkKWParameterValueFunctionEditor.h"
#include "vtkCommand.h"
#include "vtkKWEvent.h"

vtkStandardNewMacro(vtkPVAnimationCueTree);
vtkCxxRevisionMacro(vtkPVAnimationCueTree, "1.5");

//-----------------------------------------------------------------------------
vtkPVAnimationCueTree::vtkPVAnimationCueTree()
{
  this->Expanded = 0;
  this->Children = vtkCollection::New();

  this->NavigatorContainer = vtkKWFrame::New();
  this->NavigatorCanvas = vtkKWCanvas::New();
  this->NavigatorChildrenFrame = vtkKWFrame::New();

  this->TimeLineChildrenFrame = vtkKWFrame::New();

  this->SetImageType(vtkPVAnimationCueTree::IMAGE_CLOSE);
  this->ScaleChildrenOnEndPointsChange = 1;
  this->MoveEndPointsWhenChildrenChange = 1;
  this->TimeLine->SetFunctionLineStyle(
    vtkKWParameterValueFunctionEditor::LineStyleDash);
  this->TimeLine->SetPointStyle(
    vtkKWParameterValueFunctionEditor::PointStyleCursorDown);
  this->TimeLine->SetFirstPointStyle(
    vtkKWParameterValueFunctionEditor::PointStyleCursorRight);
  this->TimeLine->SetLastPointStyle(
    vtkKWParameterValueFunctionEditor::PointStyleCursorLeft);

  this->TimeLine->DisableAddAndRemoveOn(); //points cannot be added directly.
  this->SetVirtual(1);

  this->ForceBounds = 0;
  this->LastParameterBounds[0] = 0.0;
  this->LastParameterBounds[1] = 1.0;
}

//-----------------------------------------------------------------------------
vtkPVAnimationCueTree::~vtkPVAnimationCueTree()
{
  this->Children->Delete();

  this->NavigatorContainer->Delete();
  this->NavigatorCanvas->Delete();
  this->NavigatorChildrenFrame->Delete();

  this->TimeLineChildrenFrame->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::Create(vtkKWApplication* app, const char* args)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("Widget already created");
    return;
    }
  this->Superclass::Create(app, args);

  if (!this->IsCreated())
    {
    return;
    }
 
  this->NavigatorContainer->SetParent(this);
  this->NavigatorContainer->Create(app, NULL);

  this->NavigatorCanvas->SetParent(this->NavigatorContainer);
  this->NavigatorCanvas->Create(app, "-width 15 -height 0");

  this->NavigatorChildrenFrame->SetParent(this->NavigatorContainer);
  this->NavigatorChildrenFrame->Create(app, NULL);

  this->TimeLineChildrenFrame->SetParent(this->TimeLineContainer);
  this->TimeLineChildrenFrame->Create(app, NULL);

  this->Script("bind %s <ButtonPress-1> {%s ToggleExpandedState}",
    this->Image->GetWidgetName(),
    this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::AddChild(vtkPVAnimationCue* child)
{
  if (!this->IsCreated())
    {
    //TODO handle case when create is called after AddChild
    return;
    }
  if (child->IsCreated())
    {
    vtkErrorMacro("Child is already created.");
    return;
    }
//  this->TimeLine->SetFrameBackgroundColor(0.15,0.46,0.67);
  child->SetParent(this->NavigatorChildrenFrame);
  child->SetTraceReferenceObject(this);
  
  ostrstream str;
  str << "GetChild \"" << child->GetName() << "\"" << ends;
  child->SetTraceReferenceCommand(str.str());
  str.rdbuf()->freeze(0);

  child->SetTimeLineParent(this->TimeLineChildrenFrame);
  child->Create(this->GetApplication(), "-relief flat");
  child->PackWidget();
  this->Children->AddItem(child);
  this->InitializeObservers(child);
  this->DrawChildConnections(child);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::RemoveChild(vtkPVAnimationCue* child)
{
  if (!this->Children->IsItemPresent(child))
    {
    return;
    }
  // It is essential to remove focus form the child, so if any of the active cue's 
  // which is being shown in the VAnimationInterface is removed, 
  // the VAnimationInterface will let go of the cue (due to focus out).
  child->RemoveFocus();
  child->UnpackWidget();
  child->SetParent(NULL);
  this->Children->RemoveItem(child);
  this->AdjustEndPoints();
}

//-----------------------------------------------------------------------------
vtkCollectionIterator* vtkPVAnimationCueTree::NewChildrenIterator()
{
  return this->Children->NewIterator();
}

//-----------------------------------------------------------------------------
vtkPVAnimationCue* vtkPVAnimationCueTree::GetChild(const char* name)
{
  vtkCollectionIterator* iter = this->Children->NewIterator();
  vtkPVAnimationCue* t = NULL;
  
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    if (strcmp(
        vtkPVAnimationCue::SafeDownCast(iter->GetCurrentObject())->GetName(),
        name) == 0)
      {
      t = vtkPVAnimationCue::SafeDownCast(iter->GetCurrentObject());
      break;
      }
    }
  iter->Delete();
  return t;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::SetTimeMarker(double time)
{
  this->Superclass::SetTimeMarker(time);
  vtkCollectionIterator* iter = this->Children->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVAnimationCue::SafeDownCast(iter->GetCurrentObject())
      ->SetTimeMarker(time);
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::PackWidget()
{
  this->Superclass::PackWidget();
  this->Script("pack %s -side left -anchor nw -fill y -expand t",
    this->NavigatorCanvas->GetWidgetName());
  this->Script("pack %s -side left -anchor nw",
    this->NavigatorChildrenFrame->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::UnpackWidget()
{
  this->Superclass::UnpackWidget();
  this->Script("pack forget %s ",
    this->NavigatorCanvas->GetWidgetName());
  this->Script("pack forget %s ",
    this->NavigatorChildrenFrame->GetWidgetName());

}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::AdjustEndPoints()
{
  // event from one of the children of this node.
  // Iterate over all the children and get the max bounds.
  double maxbounds[2] = { -1, -1 };
  vtkCollectionIterator* iter = this->Children->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    double bounds[2];
    vtkPVAnimationCue* cue = vtkPVAnimationCue::SafeDownCast(iter->GetCurrentObject());
    if (!cue)
      {
      vtkErrorMacro("Invalid object as animation cue child");
      continue;
      }
    if (cue->GetTimeBounds(bounds))
      {
      if ( maxbounds[0]==-1 || maxbounds[0] > bounds[0] ) 
        {
        maxbounds[0] = bounds[0];
        }
      if ( maxbounds[1] == -1 || maxbounds[1] < bounds[1] )
        {
        maxbounds[1] = bounds[1];
        }
      }
    }
  this->TimeLine->DisableAddAndRemoveOff(); //points can be added directly.
  if (maxbounds[0] == -1 || maxbounds[1] == -1)
    {
    //don't show any timeline at all. i.e. remove all points in the timeline.
    this->TimeLine->RemoveAll();
    this->SetLastParameterBounds(0,0);
    }
  else
    {
    // set the two point's parameters to max bounds.
    int cur_timeline_size = this->TimeLine->GetFunctionSize();
    if (maxbounds[0] == maxbounds[1])
      {
      // add we need just one point.
      int id;
      this->TimeLine->RemoveAll();
      this->TimeLine->AddPointAtParameter(maxbounds[0], id);
      }
    else 
      {
      if (cur_timeline_size != 2)
        {
        int id;//id is dummy---needed just for the function call. 
        this->TimeLine->RemoveAll();
        this->TimeLine->AddPointAtParameter(maxbounds[0], id);
        this->TimeLine->AddPointAtParameter(maxbounds[1], id);
        }
      else if (maxbounds[0] != this->LastParameterBounds[0] || 
        maxbounds[1] != this->LastParameterBounds[1])
        {
        this->TimeLine->MoveStartToParameter(maxbounds[0], 0);
        this->TimeLine->MoveEndToParameter(maxbounds[1], 0);
        }
      }
    this->SetLastParameterBounds(maxbounds);
    }
  this->TimeLine->DisableAddAndRemoveOn(); //points cannot be added directly.
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata)
{
  if (vtkPVAnimationCue::SafeDownCast(wdg)) // Event from one of the children PVCues.
    {
    switch(event)
      {

    case vtkPVAnimationCue::KeysModifiedEvent:
      // some child has it's timeline modified. Adjust this cues end points to
      // span those of the children.
      if (this->MoveEndPointsWhenChildrenChange && !this->ForceBounds)
        {
        this->AdjustEndPoints();
        }
      else
        {
        // we don't do any moves since we forced the children to resize. 
        // But since we are indeed resizing ourselves, why not let the parent know?
        //this->InvokeEvent(vtkPVAnimationCue::KeysModifiedEvent);
        }
      break;

    case vtkKWEvent::FocusInEvent:
      // SOme child PVCue gained focus (of one of its childrent PVCues did), so 
      // clear up the focus from its siblings and tell out parent that one of our
      // children has the focus by invoking the FocusInEvent event.
      vtkPVAnimationCue* child = vtkPVAnimationCue::SafeDownCast(wdg);
      this->RemoveChildrenFocus(child);
      this->RemoveSelfFocus();  
      this->InvokeEvent(vtkKWEvent::FocusInEvent);
      break;
      }
    }
  else if (this->ScaleChildrenOnEndPointsChange && 
    vtkPVTimeLine::SafeDownCast(wdg) && (
      event == vtkKWParameterValueFunctionEditor::PointMovedEvent ||
      event == vtkKWParameterValueFunctionEditor::PointMovingEvent ))
    {
    // scale all the children.
    double new_bounds[2];
    if (this->GetTimeBounds(new_bounds))
      {
      this->ScaleChildren(this->LastParameterBounds, new_bounds);
      this->SetLastParameterBounds(new_bounds);
      }
    }
  else if (vtkPVTimeLine::SafeDownCast(wdg) && (
      event == vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent ||
      event == vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent))
    {
    double parameter[2];
    this->TimeLine->GetVisibleParameterRange(parameter);
    this->Zoom(parameter);
    this->AddTraceEntry("$kw(%s) Zoom %f %f",
      this->GetTclName(), parameter[0], parameter[1]);
    this->AddTraceEntry("update");
    }
  this->Superclass::ExecuteEvent(wdg, event, calldata);
}

  
//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::ToggleExpandedState()
{
  this->SetExpanded(!this->Expanded);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::SetExpanded(int expand)
{
  if (this->Expanded == expand)
    {
    return;
    }

  this->Expanded = expand;
  if (this->Expanded)
    {
    this->SetImageType(vtkPVAnimationCue::IMAGE_OPEN);
    this->Script("pack %s -side top -anchor nw",
      this->NavigatorContainer->GetWidgetName());

    this->Script("pack %s -side top -anchor nw -fill x -expand t",
      this->TimeLineChildrenFrame->GetWidgetName());
    }
  else
    {
    this->SetImageType(vtkPVAnimationCue::IMAGE_CLOSE);
    this->Script("pack forget %s",
      this->TimeLineChildrenFrame->GetWidgetName());

    this->Script("pack forget %s",
      this->NavigatorContainer->GetWidgetName());
    }
  this->Script("update; event generate %s <<ResizeEvent>>",
    this->GetWidgetName());
  this->AddTraceEntry("$kw(%s) SetExpanded %d", this->GetTclName(), expand);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::DrawChildConnections(vtkPVAnimationCue* )
{
  // can be done at a later stage if needed. 
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::GetFocus()
{
  if (!this->Focus)
    {
    this->Superclass::GetFocus();
    this->RemoveChildrenFocus();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::RemoveFocus()
{
  this->Superclass::RemoveFocus();
  this->RemoveChildrenFocus();
}

//-----------------------------------------------------------------------------
int vtkPVAnimationCueTree::HasFocus()
{
  if (this->Superclass::HasFocus())
    {
    return 1;
    }
  vtkCollectionIterator* iter = this->Children->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVAnimationCue* child_cue = vtkPVAnimationCue::SafeDownCast(
      iter->GetCurrentObject());
      if (child_cue->HasFocus())
        {
        return 1;
        }
    }
  iter->Delete();
  return 0;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::Zoom(double range[2])
{
  this->Superclass::Zoom(range);
  vtkCollectionIterator* iter = this->Children->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVAnimationCue* child_cue = vtkPVAnimationCue::SafeDownCast(
      iter->GetCurrentObject());
    child_cue->Zoom(range);
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::RemoveChildrenFocus(vtkPVAnimationCue* exception /*=NULL*/)
{
  vtkCollectionIterator* iter = this->Children->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVAnimationCue* child_cue = vtkPVAnimationCue::SafeDownCast(
      iter->GetCurrentObject());
    if (child_cue != exception)
      {
      child_cue->RemoveFocus();
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::ScaleChildren(double old_bounds[2], 
  double new_bounds[2])
{
  this->ForceBounds = 1;
  double range = new_bounds[1] - new_bounds[0];
  double old_range = old_bounds[1] - old_bounds[0];

  vtkCollectionIterator* iter = this->Children->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVAnimationCue* child_cue = vtkPVAnimationCue::SafeDownCast(
      iter->GetCurrentObject());
    double child_old_bounds[2];
    double child_new_bounds[2];
    if(child_cue->GetTimeBounds(child_old_bounds))
      {
      double fraction_start = (old_range)? (child_old_bounds[0] - old_bounds[0]) / old_range: 0;
      child_new_bounds[0] = new_bounds[0] + fraction_start * range;

      double fraction_end = (old_range)? (child_old_bounds[1] - old_bounds[0]) / old_range : 0;
      child_new_bounds[1] = new_bounds[0] + fraction_end * range;
      child_cue->SetTimeBounds(child_new_bounds,1);
      }
    }
  this->ForceBounds = 0;
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::SetTimeBounds(double bounds[2], int enable_scaling)
{
  this->Superclass::SetTimeBounds(bounds, enable_scaling);
  if (enable_scaling)
    {
    this->ScaleChildren(this->LastParameterBounds, bounds);
    }
  this->SetLastParameterBounds(bounds);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::InitializeStatus()
{
  this->Superclass::InitializeStatus(); 
  vtkCollectionIterator* iter = this->Children->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVAnimationCue* child_cue = vtkPVAnimationCue::SafeDownCast(
      iter->GetCurrentObject());
    child_cue->InitializeStatus(); 
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::KeyFramePropertyChanges(double ntime, int onlyFocus)
{
  this->Superclass::KeyFramePropertyChanges(ntime, onlyFocus);
  vtkCollectionIterator* iter = this->Children->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVAnimationCue* child_cue = vtkPVAnimationCue::SafeDownCast(
      iter->GetCurrentObject());
    child_cue->KeyFramePropertyChanges(ntime ,onlyFocus); 
    } 
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  vtkCollectionIterator* iter = this->Children->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVAnimationCue* child_cue = vtkPVAnimationCue::SafeDownCast(
      iter->GetCurrentObject());
    this->PropagateEnableState(child_cue);
    } 
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::SaveState(ofstream* file)
{
  this->Superclass::SaveState(file);
  *file << "$kw(" << this->GetTclName() << ") SetExpanded " << 
    this->Expanded << endl;

  // set the variable names for each child and save the child.
  vtkCollectionIterator* iter = this->Children->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVAnimationCue* child = vtkPVAnimationCue::SafeDownCast(
      iter->GetCurrentObject());
    *file << "set kw(" << child->GetTclName() << ") [$kw("
      << this->GetTclName() << ") GetChild \"" <<
      child->GetName() << "\"]" << endl;
    child->SaveState(file);
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationCueTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Expanded: " << this->Expanded << endl;
  os << indent << "NumberOfChildren: " << this->Children->GetNumberOfItems() 
    << endl;
  os << indent << "ScaleChildrenOnEndPointsChange: " << 
    this->ScaleChildrenOnEndPointsChange << endl;
  os << indent << "MoveEndPointsWhenChildrenChange: " <<
    this->MoveEndPointsWhenChildrenChange << endl;
  os << indent << "ForceBounds: " << this->ForceBounds << endl;
}
