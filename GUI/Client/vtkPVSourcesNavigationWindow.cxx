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

#include "vtkProperty.h"
#include "vtkKWApplication.h"
#include "vtkKWCanvas.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkSMPartDisplay.h"
#include "vtkPVWindow.h"
#include "vtkPVRenderView.h"

#include <stdarg.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVSourcesNavigationWindow );
vtkCxxRevisionMacro(vtkPVSourcesNavigationWindow, "1.19");

//-----------------------------------------------------------------------------
vtkPVSourcesNavigationWindow::vtkPVSourcesNavigationWindow()
{
  this->Width     = -1;
  this->Height    = -1;
  this->Canvas    = vtkKWCanvas::New();
  this->ScrollBar = vtkKWWidget::New();
  this->PopupMenu = vtkKWMenu::New();
  this->PopupModule = 0;
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
  this->PopupMenu->AddCommand("Delete", this, "PopupDeleteCallback", 0, 
       "Delete the module.  Module that are used by filters cannot be deleted.");
  char *var = this->PopupMenu->CreateCheckButtonVariable(this, "Visibility");
  this->PopupMenu->AddCheckButton("Visibility", var, 
                                  this, "PopupVisibilityCallback", 0,
                                  "Set the visibility for this module.");  
  delete [] var;

  // Representation
  this->PopupMenu->AddSeparator();
  var = this->PopupMenu->CreateRadioButtonVariable(this, "Representation");
  this->PopupMenu->AddRadioButton(VTK_OUTLINE, "Outline", var, 
      this, "PopupOutlineRepresentationCallback",
      "Outline is edges of the bounding box.");
  this->PopupMenu->AddRadioButton(VTK_SURFACE, "Surface", var,
      this, "PopupSurfaceRepresentationCallback",
      "Only external (non shared) faces of cells are displayed.");
  this->PopupMenu->AddRadioButton(VTK_WIREFRAME, "Wireframe of Surface", var,
      this, "PopupWireframeRepresentationCallback",
      "Wirefrace of surface (non shared) faces.");
  this->PopupMenu->AddRadioButton(VTK_POINTS, "Points of Surface", var,
      this, "PopupPointsRepresentationCallback",
      "Points of surface (non shared) faces.");
  delete [] var;

  // Interpolation
  this->PopupMenu->AddSeparator();
  var = this->PopupMenu->CreateRadioButtonVariable(this, "Interpolation");
  this->PopupMenu->AddRadioButton(VTK_FLAT, "Flat", var, 
                                  this, "PopupFlatInterpolationCallback",
                                  "Flat shading makes the surfaace look faceted.");
  this->PopupMenu->AddRadioButton(VTK_GOURAUD, "Gouraud", var,
      this, "PopupGouraudInterpolationCallback",
      "When the data has normals, Gouraud shading make the surface look smooth.");
  delete [] var;
  
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
void vtkPVSourcesNavigationWindow::DisplayModulePopupMenu(vtkPVSource* module, 
                                                          int x, int y)
{
  // Do not use reference counting.  This reference is short lived.
  this->PopupModule = module;

  vtkKWApplication *app = this->GetApplication();
  if ( module->IsDeletable())
    {
    this->PopupMenu->SetState("Delete", vtkKWMenu::Normal);
    }
  else
    {
    this->PopupMenu->SetState("Delete", vtkKWMenu::Disabled);
    }

  this->PopupMenu->CheckCheckButton(this, "Visibility", 
                                    module->GetVisibility());

  this->PopupMenu->CheckRadioButton(this, "Interpolation", 
            module->GetPartDisplay()->GetInterpolation());

  // Set the value of the representation radio button.
  this->PopupMenu->CheckRadioButton(this, "Representation", 
            module->GetPartDisplay()->GetRepresentation());

  // Show the popup menu in correct location (x, y is cursor position).
  this->Script("tk_popup %s %d %d", this->PopupMenu->GetWidgetName(), x, y);
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupDeleteCallback()
{
  this->PopupModule->DeleteCallback();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupVisibilityCallback()
{
  if (this->PopupMenu->GetCheckButtonValue(this, "Visibility"))
    {
    this->PopupModule->SetVisibility(1);
    }
  else
    {
    this->PopupModule->SetVisibility(0);
    }
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupFlatInterpolationCallback()
{
  this->PopupModule->GetPartDisplay()->SetInterpolation(VTK_FLAT);
  this->PopupModule->GetPVRenderView()->EventuallyRender();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupGouraudInterpolationCallback()
{
  this->PopupModule->GetPartDisplay()->SetInterpolation(VTK_GOURAUD);
  this->PopupModule->GetPVRenderView()->EventuallyRender();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupOutlineRepresentationCallback()
{
  this->PopupModule->GetPartDisplay()->SetRepresentation(VTK_OUTLINE);
  this->PopupModule->GetPVRenderView()->EventuallyRender();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupSurfaceRepresentationCallback()
{
  this->PopupModule->GetPartDisplay()->SetRepresentation(VTK_SURFACE);
  this->PopupModule->GetPVRenderView()->EventuallyRender();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupWireframeRepresentationCallback()
{
  this->PopupModule->GetPartDisplay()->SetRepresentation(VTK_WIREFRAME);
  this->PopupModule->GetPVRenderView()->EventuallyRender();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupPointsRepresentationCallback()
{
  this->PopupModule->GetPartDisplay()->SetRepresentation(VTK_POINTS);
  this->PopupModule->GetPVRenderView()->EventuallyRender();
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
