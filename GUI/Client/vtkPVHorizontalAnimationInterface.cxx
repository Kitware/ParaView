/*=========================================================================

  Program:   ParaView
  Module:    vtkPVHorizontalAnimationInterface.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVHorizontalAnimationInterface.h"

#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkPVApplication.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWFrame.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationCueTree.h"
#include "vtkCommand.h"
#include "vtkPVVerticalAnimationInterface.h"
#include "vtkKWEvent.h"
#include "vtkPVWindow.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVTimeLine.h"
#include "vtkKWParameterValueFunctionEditor.h"

vtkStandardNewMacro(vtkPVHorizontalAnimationInterface);
vtkCxxRevisionMacro(vtkPVHorizontalAnimationInterface, "1.9");

//-----------------------------------------------------------------------------
vtkPVHorizontalAnimationInterface::vtkPVHorizontalAnimationInterface()
{
  this->SplitFrame = vtkKWSplitFrame::New();
  this->TimeLineFrame = vtkKWFrame::New();
  this->PropertiesFrame = vtkKWFrame::New();
  this->ScrollFrame = vtkKWFrame::New();
  this->AnimationEntries = vtkCollection::New();
  this->AnimationEntriesIterator = this->AnimationEntries->NewIterator();
  this->ParentTree = vtkPVAnimationCueTree::New();
  this->ParentTree->SetTraceReferenceObject(this);
  this->ParentTree->SetTraceReferenceCommand("GetParentTree");
}

//-----------------------------------------------------------------------------
vtkPVHorizontalAnimationInterface::~vtkPVHorizontalAnimationInterface()
{
  this->SplitFrame->Delete();
  this->TimeLineFrame->Delete();
  this->ScrollFrame->Delete();
  this->PropertiesFrame->Delete();
  this->AnimationEntries->Delete();
  this->AnimationEntriesIterator->Delete();
  this->ParentTree->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::Create(vtkKWApplication* app, const char* args)
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);

  if (!this->Superclass::Create(pvApp, "frame", args))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
  this->ScrollFrame->SetParent(this);
  this->ScrollFrame->ScrollableOn();
  this->ScrollFrame->Create(app, "");
  this->Script("pack %s -side top -fill both -expand t",
    this->ScrollFrame->GetWidgetName());
  
  this->SplitFrame->SetParent(this->ScrollFrame->GetFrame());
  this->SplitFrame->Create(app);
  this->SplitFrame->SetFrame1Size(120);
  this->Script("bind %s <Configure> {%s ResizeCallback}",
    this->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side top -fill both -expand t",
               this->SplitFrame->GetWidgetName());

  
  this->TimeLineFrame->SetParent(this->SplitFrame->GetFrame2());
  this->TimeLineFrame->Create(app, "-relief flat");
  this->Script("pack %s -anchor n -side top -fill x -expand t",
    this->TimeLineFrame->GetWidgetName());
  this->PropertiesFrame->SetParent(this->SplitFrame->GetFrame1());
  this->PropertiesFrame->Create(app,"-relief flat");
  this->Script("pack %s -anchor n -side top -fill x -expand t",
    this->PropertiesFrame->GetWidgetName());

  this->ParentTree->SetParent(this->PropertiesFrame);
  this->ParentTree->SetTimeLineParent(this->TimeLineFrame);
  this->ParentTree->SetLabelText("Animation Scene");
  this->ParentTree->SetEnableZoom(1);
  this->ParentTree->Create(app, "-relief flat");
  this->InitializeObservers(this->ParentTree);
  this->ParentTree->PackWidget(); 
  this->ParentTree->SetExpanded(1);
  this->ParentTree->GetTimeLine()->SetParameterRangePosition(
    vtkKWParameterValueFunctionEditor::ParameterRangePositionAtTop);
  this->ParentTree->GetTimeLine()->SetCanvasOutlineStyle(
    vtkKWParameterValueFunctionEditor::CanvasOutlineStyleAllSides);
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::AddAnimationCueTree(
  vtkPVAnimationCueTree* pvCueTree)
{
  if (pvCueTree->IsCreated())
    {
    vtkErrorMacro("Child has already been created.");
    return;
    }
  this->ParentTree->AddChild(pvCueTree);
  this->Script("update");
  this->ResizeCallback();
}

//-----------------------------------------------------------------------------
int vtkPVHorizontalAnimationInterface::GetTimeBounds(double bounds[2])
{
  return this->ParentTree->GetTimeBounds(bounds);
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::SetTimeBounds(double bounds[2], 
  int enable_scaling/*=0*/)
{
  this->ParentTree->SetTimeBounds(bounds, enable_scaling);
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::SetTimeMarker(double ntime)
{
  this->ParentTree->SetTimeMarker(ntime);
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::StartRecording()
{
  this->ParentTree->StartRecording();
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::StopRecording()
{
  this->ParentTree->StopRecording();
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::RecordState(double ntime,
  double offset, int onlyFocus)
{
  this->ParentTree->RecordState(ntime, offset, onlyFocus);
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::RemoveAnimationCueTree(
  vtkPVAnimationCueTree* pvCueTree)
{
  this->Script("bind %s <<ResizeEvent>> {}",
    pvCueTree->GetWidgetName());
  this->ParentTree->RemoveChild(pvCueTree);
  this->Script("update");
  this->ResizeCallback();
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::InitializeObservers(
  vtkPVAnimationCue* cue)
{
  this->Script("bind %s <<ResizeEvent>> {%s ResizeCallback}",
    cue->GetWidgetName(), this->GetTclName());
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::ExecuteEvent(vtkObject* ,
  unsigned long , void* )
{

}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::ResizeCallback()
{
  if (!this->IsCreated())
    {
    return;
    }
// This wierd stuff is needed since the scrollable frame cannot stretch to fill
// up the parent. This leads to wierd looking GUI when the animation interface
// doesn't have too vertical many entires.
  int splitframeheight;
  int old_splitframeheight;
  this->Script("winfo height %s", this->SplitFrame->GetWidgetName());
  old_splitframeheight = vtkKWObject::GetIntegerResult(this->GetApplication());
 
  this->Script("winfo reqheight %s", this->TimeLineFrame->GetWidgetName());
  splitframeheight = vtkKWObject::GetIntegerResult(this->GetApplication());
  
  if (splitframeheight == 1 || splitframeheight == old_splitframeheight)
    {
    return;
    }
 
  int height = splitframeheight; //(parentheight > splitframeheight)? parentheight : splitframeheight;

  ostrstream str;
  str << "-height " << height << ends;
  this->SplitFrame->ConfigureOptions(str.str());
  
  str.rdbuf()->freeze(0);
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::RemoveAllKeyFrames()
{
  this->ParentTree->RemoveAllKeyFrames();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::RestoreWindowGeometry()
{
  if (this->GetApplication()->HasRegisteryValue(2, "Geometry", 
      "AnimationFrame1Size"))
    {
    int reg_size = this->GetApplication()->GetIntRegisteryValue(
      2, "Geometry", "AnimationFrame1Size");
    this->SplitFrame->SetFrame1Size(reg_size);
    }
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::SaveWindowGeometry()
{
  if (this->IsCreated())
    {
    this->GetApplication()->SetRegisteryValue(
      2, "Geometry", "AnimationFrame1Size", "%d",
      this->SplitFrame->GetFrame1Size());
    }
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::SaveState(ofstream* file)
{
  *file << endl;
  *file << "set kw(" << this->ParentTree->GetTclName() << ") [$kw("
    << this->GetTclName() << ") GetParentTree]" << endl;
  this->ParentTree->SaveState(file);
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  this->PropagateEnableState(this->ScrollFrame);
  this->PropagateEnableState(this->SplitFrame);
  this->PropagateEnableState(this->TimeLineFrame);
  this->PropagateEnableState(this->PropertiesFrame);
  this->PropagateEnableState(this->ParentTree);
}

//-----------------------------------------------------------------------------
void vtkPVHorizontalAnimationInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ParentTree: " << this->ParentTree << endl;
}
