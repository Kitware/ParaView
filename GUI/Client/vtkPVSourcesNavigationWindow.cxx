/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSourcesNavigationWindow.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSourcesNavigationWindow.h"

#include "vtkKWApplication.h"
#include "vtkKWCanvas.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"

#include <stdarg.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVSourcesNavigationWindow );
vtkCxxRevisionMacro(vtkPVSourcesNavigationWindow, "1.18");

//-----------------------------------------------------------------------------
vtkPVSourcesNavigationWindow::vtkPVSourcesNavigationWindow()
{
  this->Width     = -1;
  this->Height    = -1;
  this->Canvas    = vtkKWCanvas::New();
  this->ScrollBar = vtkKWWidget::New();
  this->PopupMenu = vtkKWMenu::New();
  this->AlwaysShowName = 0;
  this->CreateSelectionBindings = 1;
}

//-----------------------------------------------------------------------------
vtkPVSourcesNavigationWindow::~vtkPVSourcesNavigationWindow()
{
  if (this->Canvas)
    {
    this->Canvas->Delete();
    }
  if (this->ScrollBar)
    {
    this->ScrollBar->Delete();
    }
  if ( this->PopupMenu )
    {
    this->PopupMenu->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::CalculateBBox(vtkKWWidget* canvas, 
                                                 const char* name, 
                                                 int bbox[4])
{
  const char *result;
  // Get the bounding box for the name. We may need to highlight it.
  result = this->Script("%s bbox %s", canvas->GetWidgetName(), name);
  sscanf(result, "%d %d %d %d", bbox, bbox+1, bbox+2, bbox+3);
}

//-----------------------------------------------------------------------------
const char* vtkPVSourcesNavigationWindow::CreateCanvasItem(const char *format, ...)
{
  char event[16000];
  va_list var_args;
  va_start(var_args, format);
  vsprintf(event, format, var_args);
  va_end(var_args);

  return this->Script(event);
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::ChildUpdate(vtkPVSource*)
{
  vtkErrorMacro(<< "Subclass should do this.");
  vtkErrorMacro(<< "I am " << this->GetClassName());
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::Update(vtkPVSource *currentSource)
{
  // Clear the canvas
  this->Script("%s delete all", this->Canvas->GetWidgetName());

  this->ChildUpdate(currentSource);

  this->Reconfigure();
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::Reconfigure()
{
  int bbox[4];
  this->CalculateBBox(this->Canvas, "all", bbox);
  int height = atoi(this->Script("winfo height %s", 
                                 this->Canvas->GetWidgetName()));
  if (height > 1 && (bbox[3] - bbox[1]) > height)
    {
    this->Script("grid %s -row 0 -column 1 -sticky news", 
                 this->ScrollBar->GetWidgetName());
    }
  else
    {
    this->Script("grid remove %s", this->ScrollBar->GetWidgetName());
    }
  // You don't want to stick the visible part right at the border of the
  // canvas, but let some space (2 pixels on top and left)
  this->Script("%s configure -scrollregion \"%d %d %d %d\"", 
               this->Canvas->GetWidgetName(), 
               bbox[0] - 2, bbox[1] - 2, bbox[2], bbox[3]);
  this->PostChildUpdate();
}


//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", args))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  const char *wname = this->GetWidgetName();
  ostrstream opts;
  
  if (this->Width > 0 && this->Height > 0)
    {
    opts << " -width " << this->Width << " -height " << this->Height;
    }
  else if (this->Width > 0)
    {
    opts << " -width " << this->Width;
    }
  else if (this->Height > 0)
    {
    opts << " -height " << this->Height;
    }

  opts << " -highlightthickness 0";
  opts << " -bg white" << ends;

  char* optstr = opts.str();
  this->Canvas->SetParent(this);
  this->Canvas->Create(this->GetApplication(), optstr); 
  delete[] optstr;

  ostrstream command;
  this->ScrollBar->SetParent(this);
  command << "-command \"" <<  this->Canvas->GetWidgetName()
          << " yview\"" << ends;
  char* commandStr = command.str();
  this->ScrollBar->Create(this->GetApplication(), "scrollbar", commandStr);
  delete[] commandStr;

  this->Script("%s configure -yscrollcommand \"%s set\"", 
               this->Canvas->GetWidgetName(),
               this->ScrollBar->GetWidgetName());

  this->Canvas->SetBind(this, "<Configure>", "Reconfigure");

  this->Script("grid %s -row 0 -column 0 -sticky news", 
               this->Canvas->GetWidgetName());
  this->Script("grid columnconfig %s 0 -weight 1", wname);
  this->Script("grid rowconfig %s 0 -weight 1", wname);
  this->PopupMenu->SetParent(this);
  this->PopupMenu->Create(this->GetApplication(), "-tearoff 0");
  this->PopupMenu->AddCommand("Delete", this, "DeleteWidget", 0, 
                              "Delete current widget");
  char *var = this->PopupMenu->CreateCheckButtonVariable(this, "Visibility");
  this->PopupMenu->AddCheckButton("Visibility", var, this, "Visibility", 0,
                                  "Set visibility for the current object");  
  delete [] var;
  this->PopupMenu->AddCascade("Representation", 0, 0);
  this->PopupMenu->AddCascade("Interpolation", 0, 0);
  /*
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  this->PopupMenu->AddCascade(
    "VTK Filters", pvApp->GetMainWindow()->GetFilterMenu(),
    4, "Choose a filter from a list of VTK filters");
  */
  this->ChildCreate();
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::SetWidth(int width)
{
  if (this->Width == width)
    {
    return;
    }

  this->Modified();
  this->Width = width;

  if (this->IsCreated())
    {
    this->Script("%s configure -width %d", this->Canvas->GetWidgetName(), 
                    width);
    }
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::SetHeight(int height)
{
  if (this->Height == height)
    {
    return;
    }

  this->Modified();
  this->Height = height;

  if (this->IsCreated())
    {
    this->Script("%s configure -height %d", this->Canvas->GetWidgetName(), 
                 height);
    }
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::SetAlwaysShowName(int val)
{
  if (this->AlwaysShowName == val)
    {
    return;
    }

  this->AlwaysShowName = val;
  this->Modified();

  if (this->GetApplication())
    {
    vtkPVApplication* app = vtkPVApplication::SafeDownCast(this->GetApplication());
    if (app)
      {
      vtkPVWindow* window = app->GetMainWindow();
      if (window && window->GetCurrentPVSource())
        {
        this->Update(window->GetCurrentPVSource());
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::HighlightObject(const char* widget, int onoff)
{
  this->Script("%s itemconfigure %s -fill %s", 
               this->Canvas->GetWidgetName(), widget,
               (onoff ? "red" : "blue") );
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::DisplayModulePopupMenu(const char* module, 
                                                   int x, int y)
{
  //cout << "Popup for module: " << module << " at " << x << ", " << y << endl;
  vtkKWApplication *app = this->GetApplication();
  ostrstream str;
  if ( app->EvaluateBooleanExpression("%s IsDeletable", module) )
    {
    str << "ExecuteCommandOnModule " << module << " DeleteCallback" << ends;
    this->PopupMenu->SetEntryCommand("Delete", this, str.str());
    this->PopupMenu->SetState("Delete", vtkKWMenu::Normal);
    }
  else
    {
    this->PopupMenu->SetState("Delete", vtkKWMenu::Disabled);
    }
  str.rdbuf()->freeze(0);
  ostrstream str1;
  if ( !app->EvaluateBooleanExpression("%s GetHideDisplayPage", module) )
    {
    this->PopupMenu->SetState("Visibility", vtkKWMenu::Normal);
    this->PopupMenu->SetState("Representation", vtkKWMenu::Normal);
    this->PopupMenu->SetState("Interpolation", vtkKWMenu::Normal);
    char *var = this->PopupMenu->CreateCheckButtonVariable(this, "Visibility");
    str1 << " " << module << " SetVisibility $" 
         << var << ";"
         << "[ [ $Application GetMainWindow ] GetMainView ] EventuallyRender" 
         <<  ends;
    this->PopupMenu->SetEntryCommand("Visibility", str1.str());
    if ( app->EvaluateBooleanExpression("%s GetVisibility", module) )
      {
      this->Script("set %s 1", var);
      }
    else
      {
      this->Script("set %s 0", var);
      }
    delete [] var;
    this->Script("%s SetCascade [ %s GetIndex \"Representation\" ] "
                 "[ [ [ [ %s GetPVOutput ] GetRepresentationMenu ] "
                 "GetMenu ] GetWidgetName ]",
                 this->PopupMenu->GetTclName(),
                 this->PopupMenu->GetTclName(), module);

    this->Script("%s SetCascade [ %s GetIndex \"Interpolation\" ] "
                 "[ [ [ [ %s GetPVOutput ] GetInterpolationMenu ] "
                 "GetMenu ] GetWidgetName ]",
                 this->PopupMenu->GetTclName(),
                 this->PopupMenu->GetTclName(), module);
                 
    }
  else
    {
    this->PopupMenu->SetState("Visibility", vtkKWMenu::Disabled);
    this->PopupMenu->SetState("Representation", vtkKWMenu::Disabled);
    this->PopupMenu->SetState("Interpolation", vtkKWMenu::Disabled);
    }
  this->Script("tk_popup %s %d %d", this->PopupMenu->GetWidgetName(), x, y);
  str1.rdbuf()->freeze(0);
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::ExecuteCommandOnModule(
  const char* module, const char* command)
{
  //cout << "Executing: " << command << " on module: " << module << endl;
  this->Script("%s %s", module, command);
}

//-----------------------------------------------------------------------------
char* vtkPVSourcesNavigationWindow::GetTextRepresentation(vtkPVSource* comp)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  return pvApp->GetTextRepresentation(comp);
}

//----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Canvas);
  this->PropagateEnableState(this->ScrollBar);
  this->PropagateEnableState(this->PopupMenu);
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Canvas: " << this->GetCanvas() << endl;
  os << indent << "AlwaysShowName: " << this->AlwaysShowName << endl;
 os << indent << "CreateSelectionBindings: " << this->CreateSelectionBindings << endl;
}
