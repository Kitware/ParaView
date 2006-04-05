/*=========================================================================

  Module:    vtkKWDragAndDropTargetSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWDragAndDropTargetSet.h"

#include "vtkObjectFactory.h"
#include "vtkKWCoreWidget.h"
#include "vtkKWTkUtilities.h"

#include <vtksys/stl/list>
#include <vtksys/stl/algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWDragAndDropTargetSet );
vtkCxxRevisionMacro(vtkKWDragAndDropTargetSet, "1.15");

//----------------------------------------------------------------------------
class vtkKWDragAndDropTargetSetInternals
{
public:

  typedef vtksys_stl::list<vtkKWDragAndDropTargetSet::TargetSlot*> TargetsContainer;
  typedef vtksys_stl::list<vtkKWDragAndDropTargetSet::TargetSlot*>::iterator TargetsContainerIterator;

  TargetsContainer Targets;
};

//----------------------------------------------------------------------------
vtkKWDragAndDropTargetSet::TargetSlot::TargetSlot()
{
  this->Target         = NULL;
  this->StartCommand   = NULL;
  this->PerformCommand = NULL;
  this->EndCommand     = NULL;
}

//----------------------------------------------------------------------------
vtkKWDragAndDropTargetSet::TargetSlot::~TargetSlot()
{
  this->Target = NULL;
  this->SetStartCommand(NULL);
  this->SetPerformCommand(NULL);
  this->SetEndCommand(NULL);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::TargetSlot::SetStartCommand(const char *arg)
{
  if ((this->StartCommand == NULL && arg == NULL) ||
      (this->StartCommand && arg && (!strcmp(this->StartCommand,arg))))
    { 
    return;
    }

  if (this->StartCommand) 
    { 
    delete [] this->StartCommand; 
    }

  if (arg)
    {
    this->StartCommand = new char[strlen(arg) + 1];
    strcpy(this->StartCommand, arg);
    }
   else
    {
    this->StartCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::TargetSlot::SetPerformCommand(const char *arg)
{
  if ((this->PerformCommand == NULL && arg == NULL) ||
      (this->PerformCommand && arg && (!strcmp(this->PerformCommand,arg))))
    { 
    return;
    }

  if (this->PerformCommand) 
    { 
    delete [] this->PerformCommand; 
    }

  if (arg)
    {
    this->PerformCommand = new char[strlen(arg) + 1];
    strcpy(this->PerformCommand, arg);
    }
   else
    {
    this->PerformCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::TargetSlot::SetEndCommand(const char *arg)
{
  if ((this->EndCommand == NULL && arg == NULL) ||
      (this->EndCommand && arg && (!strcmp(this->EndCommand,arg))))
    { 
    return;
    }

  if (this->EndCommand) 
    { 
    delete [] this->EndCommand; 
    }

  if (arg)
    {
    this->EndCommand = new char[strlen(arg) + 1];
    strcpy(this->EndCommand, arg);
    }
   else
    {
    this->EndCommand = NULL;
    }
}

//----------------------------------------------------------------------------
vtkKWDragAndDropTargetSet::vtkKWDragAndDropTargetSet()
{
  this->Enable         = 1;
  this->Source         = NULL;
  this->SourceAnchor   = NULL;

  this->StartCommand   = NULL;
  this->PerformCommand = NULL;
  this->EndCommand     = NULL;

  this->Internals = new vtkKWDragAndDropTargetSetInternals;
}

//----------------------------------------------------------------------------
vtkKWDragAndDropTargetSet::~vtkKWDragAndDropTargetSet()
{
  this->Source = NULL;
  this->SourceAnchor = NULL;

  if (this->StartCommand)
    {
    delete [] this->StartCommand;
    this->StartCommand = NULL;
    }
  if (this->PerformCommand)
    {
    delete [] this->PerformCommand;
    this->PerformCommand = NULL;
    }
  if (this->EndCommand)
    {
    delete [] this->EndCommand;
    this->EndCommand = NULL;
    }

  this->DeleteAllTargets();
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::DeleteAllTargets()
{
  if (this->Internals)
    {
    vtkKWDragAndDropTargetSetInternals::TargetsContainerIterator it = 
      this->Internals->Targets.begin();
    vtkKWDragAndDropTargetSetInternals::TargetsContainerIterator end = 
      this->Internals->Targets.end();
    for (; it != end; ++it)
      {
      if (*it)
        {
        delete *it;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::SetEnable(int arg)
{
  if (this->Enable == arg)
    {
    return;
    }

  this->Enable = arg;
  this->Modified();

  if (arg)
    {
    this->AddBindings();
    }
  else
    {
    this->RemoveBindings();
    }
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::SetSource(vtkKWWidget *arg)
{
  if (this->Source == arg)
    {
    return;
    }

  this->RemoveBindings();

  this->Source = arg;
  this->Modified();

  this->AddBindings();
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::SetSourceAnchor(vtkKWWidget *arg)
{
  if (this->SourceAnchor == arg)
    {
    return;
    }

  this->RemoveBindings();

  this->SourceAnchor = arg;
  this->Modified();

  this->AddBindings();
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::InvokeCommandWithCoordinates(
  const char *command, int x, int y)
{
  if (command && *command)
    {
    this->Script("%s %d %d", command, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::SetStartCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->StartCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::InvokeStartCommand(int x, int y)
{
  this->InvokeCommandWithCoordinates(this->StartCommand, x, y);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::SetPerformCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PerformCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::InvokePerformCommand(int x, int y)
{
  this->InvokeCommandWithCoordinates(this->PerformCommand, x, y);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::SetEndCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->EndCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::InvokeEndCommand(int x, int y)
{
  this->InvokeCommandWithCoordinates(this->EndCommand, x, y);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::AddBindings()
{
  if (!this->GetApplication())
    {
    vtkErrorMacro("Error! Application not set!");
    return;
    }

  vtkKWWidget *anchor = this->SourceAnchor ? this->SourceAnchor : this->Source;
  if (!anchor || !anchor->IsCreated())
    {
    return;
    }
  
  anchor->AddBinding("<Button-1>", this, "StartCallback %X %Y");
  anchor->AddBinding("<B1-Motion>", this, "PerformCallback %X %Y");
  anchor->AddBinding("<ButtonRelease-1>", this, "EndCallback %X %Y");
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::RemoveBindings()
{
  if (!this->GetApplication())
    {
    vtkErrorMacro("Error! Application not set!");
    return;
    }

  vtkKWWidget *anchor = this->SourceAnchor ? this->SourceAnchor : this->Source;
  if (!anchor || !anchor->IsCreated())
    {
    return;
    }

  anchor->RemoveBinding("<Button-1>");
  anchor->RemoveBinding("<B1-Motion>");
  anchor->RemoveBinding("<ButtonRelease-1>");
}

//----------------------------------------------------------------------------
vtkKWDragAndDropTargetSet::TargetSlot*
vtkKWDragAndDropTargetSet::GetTarget(vtkKWWidget *widget)
{
  if (this->Internals)
    {
    vtkKWDragAndDropTargetSetInternals::TargetsContainerIterator it = 
      this->Internals->Targets.begin();
    vtkKWDragAndDropTargetSetInternals::TargetsContainerIterator end = 
      this->Internals->Targets.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Target == widget)
        {
        return (*it);
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargetSet::AddTarget(vtkKWWidget *widget)
{
  vtkKWDragAndDropTargetSet::TargetSlot *found = this->GetTarget(widget);
  if (found)
    {
    vtkErrorMacro("The Drag & Drop target already exists.");
    return 0;
    }

  vtkKWDragAndDropTargetSet::TargetSlot *target_slot = 
    new vtkKWDragAndDropTargetSet::TargetSlot;
  this->Internals->Targets.push_back(target_slot);
  target_slot->Target = widget;

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargetSet::RemoveTarget(vtkKWWidget *widget)
{
  vtkKWDragAndDropTargetSet::TargetSlot *found = this->GetTarget(widget);
  if (!found)
    {
    return 0;
    }
  
  vtkKWDragAndDropTargetSetInternals::TargetsContainerIterator pos = 
    vtksys_stl::find(this->Internals->Targets.begin(),
                 this->Internals->Targets.end(),
                 found);
  
  if (pos == this->Internals->Targets.end())
    {
    vtkErrorMacro("Error while searching for a Drag & Drop target.");
    return 0;
    }

  this->Internals->Targets.erase(pos);
  delete found;

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargetSet::HasTarget(vtkKWWidget *widget)
{
  vtkKWDragAndDropTargetSet::TargetSlot *found = this->GetTarget(widget);
  return found ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargetSet::GetNumberOfTargets()
{
  return this->Internals ? this->Internals->Targets.size() : 0;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargetSet::SetTargetStartCommand(vtkKWWidget *target, 
                                                     vtkObject *object, 
                                                     const char *method)
{
  if (!target || !object || !method || !method[0])
    {
    return 0;
    }

  vtkKWDragAndDropTargetSet::TargetSlot *found = this->GetTarget(target);
  if (!found)
    {
    this->AddTarget(target);
    }
  found = this->GetTarget(target);
  if (!found)
    {
    return 0;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, object, method);
  found->SetStartCommand(command);
  delete [] command;

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargetSet::SetTargetPerformCommand(vtkKWWidget *target, 
                                                       vtkObject *object, 
                                                       const char *method)
{
  if (!target || !object || !method || !method[0])
    {
    return 0;
    }

  vtkKWDragAndDropTargetSet::TargetSlot *found = this->GetTarget(target);
  if (!found)
    {
    this->AddTarget(target);
    }
  found = this->GetTarget(target);
  if (!found)
    {
    return 0;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, object, method);
  found->SetPerformCommand(command);
  delete [] command;

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargetSet::SetTargetEndCommand(vtkKWWidget *target, 
                                                   vtkObject *object, 
                                                   const char *method)
{
  if (!target || !object || !method || !method[0])
    {
    return 0;
    }

  vtkKWDragAndDropTargetSet::TargetSlot *found = this->GetTarget(target);
  if (!found)
    {
    this->AddTarget(target);
    }
  found = this->GetTarget(target);
  if (!found)
    {
    return 0;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, object, method);
  found->SetEndCommand(command);
  delete [] command;

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::StartCallback(int x, int y)
{
  if (!this->Enable)
    {
    return;
    }

  this->InvokeStartCommand(x, y);

  if (this->Internals && this->GetNumberOfTargets())
    {
    // Set the cursor and invert foreground/background to better show what
    // is dragged

    vtkKWWidget *anchor = 
      this->SourceAnchor ? this->SourceAnchor : this->Source;
    if (anchor && anchor->IsCreated())
      {
      vtkKWTkUtilities::SetTopLevelMouseCursor(anchor, "hand2");
      vtkKWCoreWidget *anchor_as_core = vtkKWCoreWidget::SafeDownCast(anchor);
      if (anchor_as_core->HasConfigurationOption("-fg") &&
          anchor_as_core->HasConfigurationOption("-bg"))
        {
        double fr, fg, fb, br, bg, bb;
        anchor_as_core->GetConfigurationOptionAsColor("-fg", &fr, &fg, &fb);
        anchor_as_core->GetConfigurationOptionAsColor("-bg", &br, &bg, &bb);
        anchor_as_core->SetConfigurationOptionAsColor("-fg", br, bg, bb);
        anchor_as_core->SetConfigurationOptionAsColor("-bg", fr, fg, fb);
        }
      }

    // Call each target's StartCommand

    vtkKWDragAndDropTargetSetInternals::TargetsContainerIterator it = 
      this->Internals->Targets.begin();
    vtkKWDragAndDropTargetSetInternals::TargetsContainerIterator end = 
      this->Internals->Targets.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->StartCommand && *(*it)->StartCommand)
        {
        if (this->Source && !this->Source->GetApplication())
          {
          vtkErrorMacro("Error! Source's application not set!");
          continue;
          }
        if (this->SourceAnchor && !this->SourceAnchor->GetApplication())
          {
          vtkErrorMacro("Error! SourceAnchor's application not set!");
          continue;
          }
        this->Script("%s %d %d %s %s", 
                     (*it)->StartCommand, x, y, 
                     this->Source ? this->Source->GetTclName() : "", 
                     this->SourceAnchor ? this->SourceAnchor->GetTclName():"");
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::PerformCallback(int x, int y)
{
  if (!this->Enable)
    {
    return;
    }

  this->InvokePerformCommand(x, y);

  if (this->Internals && this->GetNumberOfTargets())
    {
    // Call each target's PerformCommand

    vtkKWDragAndDropTargetSetInternals::TargetsContainerIterator it = 
      this->Internals->Targets.begin();
    vtkKWDragAndDropTargetSetInternals::TargetsContainerIterator end = 
      this->Internals->Targets.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->PerformCommand && *(*it)->PerformCommand)
        {
        if (this->Source && !this->Source->GetApplication())
          {
          vtkErrorMacro("Error! Source's application not set!");
          continue;
          }
        if (this->SourceAnchor && !this->SourceAnchor->GetApplication())
          {
          vtkErrorMacro("Error! SourceAnchor's application not set!");
          continue;
          }
        this->Script("%s %d %d %s %s", 
                     (*it)->PerformCommand, x, y, 
                     this->Source ? this->Source->GetTclName() : "", 
                     this->SourceAnchor ? this->SourceAnchor->GetTclName():"");
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::EndCallback(int x, int y)
{
  if (!this->Enable)
    {
    return;
    }

  if (this->Internals && this->GetNumberOfTargets())
    {
    // Reset the cursor and the background/foreground colors

    vtkKWWidget *anchor = 
      this->SourceAnchor ? this->SourceAnchor : this->Source;
    if (anchor && anchor->IsCreated())
      {
      vtkKWTkUtilities::SetTopLevelMouseCursor(anchor, NULL);
      vtkKWCoreWidget *anchor_as_core = vtkKWCoreWidget::SafeDownCast(anchor);
      if (anchor_as_core->HasConfigurationOption("-fg") &&
          anchor_as_core->HasConfigurationOption("-bg"))
        {
        double fr, fg, fb, br, bg, bb;
        anchor_as_core->GetConfigurationOptionAsColor("-fg", &fr, &fg, &fb);
        anchor_as_core->GetConfigurationOptionAsColor("-bg", &br, &bg, &bb);
        anchor_as_core->SetConfigurationOptionAsColor("-fg", br, bg, bb);
        anchor_as_core->SetConfigurationOptionAsColor("-bg", fr, fg, fb);
        }
      }

    // Find if the cursor is in a target, and call its EndCommand

    vtkKWDragAndDropTargetSetInternals::TargetsContainerIterator it = 
      this->Internals->Targets.begin();
    vtkKWDragAndDropTargetSetInternals::TargetsContainerIterator end = 
      this->Internals->Targets.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->EndCommand && *(*it)->EndCommand &&
          (*it)->Target && 
          (*it)->Target->IsCreated() &&
          vtkKWTkUtilities::ContainsCoordinates((*it)->Target, x, y))
        {
        if (this->Source && !this->Source->GetApplication())
          {
          vtkErrorMacro("Error! Source's application not set!");
          continue;
          }
        if (this->SourceAnchor && !this->SourceAnchor->GetApplication())
          {
          vtkErrorMacro("Error! SourceAnchor's application not set!");
          continue;
          }
        this->Script("%s %d %d %s %s %s", 
                     (*it)->EndCommand, x, y, 
                     this->Source ? this->Source->GetTclName() : "", 
                     this->SourceAnchor ? this->SourceAnchor->GetTclName():"",
                     (*it)->Target->GetTclName());
        }
      }
    }

  this->InvokeEndCommand(x, y);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargetSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Enable: " 
     << (this->Enable ? "On" : "Off") << endl;
  os << indent << "Source: " << this->Source << endl;
  os << indent << "SourceAnchor: " << this->SourceAnchor << endl;
}
