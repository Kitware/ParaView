/*=========================================================================

  Module:    vtkKWDragAndDropTargets.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWDragAndDropTargets.h"

#include "vtkObjectFactory.h"
#include "vtkKWWidget.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWApplication.h"

#include <vtkstd/list>
#include <vtkstd/algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWDragAndDropTargets );
vtkCxxRevisionMacro(vtkKWDragAndDropTargets, "1.1");

int vtkKWDragAndDropTargetsCommand(ClientData cd, Tcl_Interp *interp,
                                   int argc, char *argv[]);

//----------------------------------------------------------------------------
class vtkKWDragAndDropTargetsInternals
{
public:

  typedef vtkstd::list<vtkKWDragAndDropTargets::TargetSlot*> TargetsContainer;
  typedef vtkstd::list<vtkKWDragAndDropTargets::TargetSlot*>::iterator TargetsContainerIterator;

  TargetsContainer Targets;
};

//----------------------------------------------------------------------------
vtkKWDragAndDropTargets::TargetSlot::TargetSlot()
{
  this->Target         = NULL;
  this->StartCommand   = NULL;
  this->PerformCommand = NULL;
  this->EndCommand     = NULL;
}

//----------------------------------------------------------------------------
vtkKWDragAndDropTargets::TargetSlot::~TargetSlot()
{
  this->Target = NULL;
  this->SetStartCommand(NULL);
  this->SetPerformCommand(NULL);
  this->SetEndCommand(NULL);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargets::TargetSlot::SetStartCommand(const char *arg)
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
void vtkKWDragAndDropTargets::TargetSlot::SetPerformCommand(const char *arg)
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
void vtkKWDragAndDropTargets::TargetSlot::SetEndCommand(const char *arg)
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
vtkKWDragAndDropTargets::vtkKWDragAndDropTargets()
{
  this->Enable         = 1;
  this->Source         = NULL;
  this->Anchor         = NULL;

  this->StartCommand   = NULL;
  this->PerformCommand = NULL;
  this->EndCommand     = NULL;

  this->Internals = new vtkKWDragAndDropTargetsInternals;
}

//----------------------------------------------------------------------------
vtkKWDragAndDropTargets::~vtkKWDragAndDropTargets()
{
  this->Source = NULL;
  this->Anchor = NULL;

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
void vtkKWDragAndDropTargets::DeleteAllTargets()
{
  if (this->Internals)
    {
    vtkKWDragAndDropTargetsInternals::TargetsContainerIterator it = 
      this->Internals->Targets.begin();
    vtkKWDragAndDropTargetsInternals::TargetsContainerIterator end = 
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
void vtkKWDragAndDropTargets::SetEnable(int arg)
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
void vtkKWDragAndDropTargets::SetSource(vtkKWWidget *arg)
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
void vtkKWDragAndDropTargets::SetAnchor(vtkKWWidget *arg)
{
  if (this->Anchor == arg)
    {
    return;
    }

  this->RemoveBindings();

  this->Anchor = arg;
  this->Modified();

  this->AddBindings();
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargets::SetStartCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->StartCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargets::SetPerformCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->PerformCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargets::SetEndCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->EndCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargets::AddBindings()
{
  if (!this->GetApplication())
    {
    vtkErrorMacro("Error! Application not set!");
    return;
    }

  vtkKWWidget *anchor = this->Anchor ? this->Anchor : this->Source;
  if (!anchor || !anchor->IsCreated())
    {
    return;
    }
  
  this->Script("bind %s <Button-1> {+ %s StartCallback %%X %%Y}",
               anchor->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <B1-Motion> {+ %s PerformCallback %%X %%Y}",
               anchor->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <ButtonRelease-1> {+ %s EndCallback %%X %%Y}",
               anchor->GetWidgetName(), this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargets::RemoveBindings()
{
  if (!this->GetApplication())
    {
    vtkErrorMacro("Error! Application not set!");
    return;
    }

  vtkKWWidget *anchor = this->Anchor ? this->Anchor : this->Source;
  if (!anchor || !anchor->IsCreated())
    {
    return;
    }

  this->Script("bind %s <Button-1> {}",
               anchor->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <B1-Motion> {}",
               anchor->GetWidgetName(), this->GetTclName());
  this->Script("bind %s <ButtonRelease-1> {}",
               anchor->GetWidgetName(), this->GetTclName());
}

//----------------------------------------------------------------------------
vtkKWDragAndDropTargets::TargetSlot*
vtkKWDragAndDropTargets::GetTarget(vtkKWWidget *widget)
{
  if (this->Internals)
    {
    vtkKWDragAndDropTargetsInternals::TargetsContainerIterator it = 
      this->Internals->Targets.begin();
    vtkKWDragAndDropTargetsInternals::TargetsContainerIterator end = 
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
int vtkKWDragAndDropTargets::AddTarget(vtkKWWidget *widget)
{
  vtkKWDragAndDropTargets::TargetSlot *found = this->GetTarget(widget);
  if (found)
    {
    vtkErrorMacro("The Drag & Drop target already exists.");
    return 0;
    }

  vtkKWDragAndDropTargets::TargetSlot *target_slot = 
    new vtkKWDragAndDropTargets::TargetSlot;
  this->Internals->Targets.push_back(target_slot);
  target_slot->Target = widget;

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargets::RemoveTarget(vtkKWWidget *widget)
{
  vtkKWDragAndDropTargets::TargetSlot *found = this->GetTarget(widget);
  if (!found)
    {
    return 0;
    }
  
  vtkKWDragAndDropTargetsInternals::TargetsContainerIterator pos = 
    vtkstd::find(this->Internals->Targets.begin(),
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
int vtkKWDragAndDropTargets::HasTarget(vtkKWWidget *widget)
{
  vtkKWDragAndDropTargets::TargetSlot *found = this->GetTarget(widget);
  return found ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargets::GetNumberOfTargets()
{
  return this->Internals ? this->Internals->Targets.size() : 0;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargets::SetTargetStartCommand(vtkKWWidget *target, 
                                                   vtkKWObject *object, 
                                                   const char *method)
{
  if (!target || !object || !method || !method[0])
    {
    return 0;
    }

  if (!object->GetApplication())
    {
    vtkErrorMacro("Error! Object's application not set!");
    return 0;
    }

  vtkKWDragAndDropTargets::TargetSlot *found = this->GetTarget(target);
  if (!found)
    {
    this->AddTarget(target);
    }
  found = this->GetTarget(target);
  if (!found)
    {
    return 0;
    }

  ostrstream command;
  command << object->GetTclName() << " " << method << ends;
  found->SetStartCommand(command.str());
  command.rdbuf()->freeze(0);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargets::SetTargetPerformCommand(vtkKWWidget *target, 
                                                     vtkKWObject *object, 
                                                     const char *method)
{
  if (!target || !object || !method || !method[0])
    {
    return 0;
    }

  if (!object->GetApplication())
    {
    vtkErrorMacro("Error! Object's application not set!");
    return 0;
    }

  vtkKWDragAndDropTargets::TargetSlot *found = this->GetTarget(target);
  if (!found)
    {
    this->AddTarget(target);
    }
  found = this->GetTarget(target);
  if (!found)
    {
    return 0;
    }

  ostrstream command;
  command << object->GetTclName() << " " << method << ends;
  found->SetPerformCommand(command.str());
  command.rdbuf()->freeze(0);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWDragAndDropTargets::SetTargetEndCommand(vtkKWWidget *target, 
                                           vtkKWObject *object, 
                                           const char *method)
{
  if (!target || !object || !method || !method[0])
    {
    return 0;
    }

  if (!object->GetApplication())
    {
    vtkErrorMacro("Error! Object's application not set!");
    return 0;
    }

  vtkKWDragAndDropTargets::TargetSlot *found = this->GetTarget(target);
  if (!found)
    {
    this->AddTarget(target);
    }
  found = this->GetTarget(target);
  if (!found)
    {
    return 0;
    }

  ostrstream command;
  command << object->GetTclName() << " " << method << ends;
  found->SetEndCommand(command.str());
  command.rdbuf()->freeze(0);

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargets::StartCallback(int x, int y)
{
  if (!this->Enable)
    {
    return;
    }

  if (this->StartCommand)
    {
    this->Script("eval %s %d %d", this->StartCommand, x, y);
    }

  if (this->Internals && this->GetNumberOfTargets())
    {
    // Set the cursor and invert foreground/background to better show what
    // is dragged

    vtkKWWidget *anchor = this->Anchor ? this->Anchor : this->Source;
    if (anchor && anchor->IsCreated())
      {
      this->Script("[winfo toplevel %s] config -cursor hand2", 
                   anchor->GetWidgetName());

      if (anchor->HasConfigurationOption("-fg") &&
          anchor->HasConfigurationOption("-bg"))
        {
        int fr, fg, fb, br, bg, bb;
        anchor->GetForegroundColor(&fr, &fg, &fb);
        anchor->GetBackgroundColor(&br, &bg, &bb);
        anchor->SetForegroundColor(br, bg, bb);
        anchor->SetBackgroundColor(fr, fg, fb);
        }
      }

    // Call each target's StartCommand

    vtkKWDragAndDropTargetsInternals::TargetsContainerIterator it = 
      this->Internals->Targets.begin();
    vtkKWDragAndDropTargetsInternals::TargetsContainerIterator end = 
      this->Internals->Targets.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->StartCommand)
        {
        if (this->Source && !this->Source->GetApplication())
          {
          vtkErrorMacro("Error! Source's application not set!");
          continue;
          }
        if (this->Anchor && !this->Anchor->GetApplication())
          {
          vtkErrorMacro("Error! Anchor's application not set!");
          continue;
          }
        this->Script("eval %s %d %d %s %s", 
                     (*it)->StartCommand, x, y, 
                     this->Source ? this->Source->GetTclName() : "", 
                     this->Anchor ? this->Anchor->GetTclName() : "");
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargets::PerformCallback(int x, int y)
{
  if (!this->Enable)
    {
    return;
    }

  if (this->PerformCommand)
    {
    this->Script("eval %s %d %d", this->PerformCommand, x, y);
    }

  if (this->Internals && this->GetNumberOfTargets())
    {
    // Call each target's PerformCommand

    vtkKWDragAndDropTargetsInternals::TargetsContainerIterator it = 
      this->Internals->Targets.begin();
    vtkKWDragAndDropTargetsInternals::TargetsContainerIterator end = 
      this->Internals->Targets.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->PerformCommand)
        {
        if (this->Source && !this->Source->GetApplication())
          {
          vtkErrorMacro("Error! Source's application not set!");
          continue;
          }
        if (this->Anchor && !this->Anchor->GetApplication())
          {
          vtkErrorMacro("Error! Anchor's application not set!");
          continue;
          }
        this->Script("eval %s %d %d %s %s", 
                     (*it)->PerformCommand, x, y, 
                     this->Source ? this->Source->GetTclName() : "", 
                     this->Anchor ? this->Anchor->GetTclName() : "");
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargets::EndCallback(int x, int y)
{
  if (!this->Enable)
    {
    return;
    }

  if (this->Internals && this->GetNumberOfTargets())
    {
    // Reset the cursor and the background/foreground colors

    vtkKWWidget *anchor = this->Anchor ? this->Anchor : this->Source;
    if (anchor && anchor->IsCreated())
      {
      this->Script("[winfo toplevel %s] config -cursor {}", 
                   anchor->GetWidgetName());

      if (anchor->HasConfigurationOption("-fg") &&
          anchor->HasConfigurationOption("-bg"))
        {
        int fr, fg, fb, br, bg, bb;
        anchor->GetForegroundColor(&fr, &fg, &fb);
        anchor->GetBackgroundColor(&br, &bg, &bb);
        anchor->SetForegroundColor(br, bg, bb);
        anchor->SetBackgroundColor(fr, fg, fb);
        }
      }

    // Find if the cursor is in a target, and call its EndCommand

    vtkKWDragAndDropTargetsInternals::TargetsContainerIterator it = 
      this->Internals->Targets.begin();
    vtkKWDragAndDropTargetsInternals::TargetsContainerIterator end = 
      this->Internals->Targets.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->EndCommand && 
          (*it)->Target && 
          (*it)->Target->IsCreated() &&
          vtkKWTkUtilities::ContainsCoordinates(
            (*it)->Target->GetApplication()->GetMainInterp(),
            (*it)->Target->GetWidgetName(),
            x, y))
        {
        if (this->Source && !this->Source->GetApplication())
          {
          vtkErrorMacro("Error! Source's application not set!");
          continue;
          }
        if (this->Anchor && !this->Anchor->GetApplication())
          {
          vtkErrorMacro("Error! Anchor's application not set!");
          continue;
          }
        this->Script("eval %s %d %d %s %s %s", 
                     (*it)->EndCommand, x, y, 
                     this->Source ? this->Source->GetTclName() : "", 
                     this->Anchor ? this->Anchor->GetTclName() : "",
                     (*it)->Target->GetTclName());
        }
      }
    }

  if (this->EndCommand)
    {
    this->Script("eval %s %d %d", this->EndCommand, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkKWDragAndDropTargets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Enable: " 
     << (this->Enable ? "On" : "Off") << endl;
  os << indent << "Source: " << this->Source << endl;
  os << indent << "Anchor: " << this->Anchor << endl;
}
