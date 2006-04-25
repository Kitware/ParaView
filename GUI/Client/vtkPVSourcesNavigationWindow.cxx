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
#include "vtkKWFrameWithLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWScrollbar.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkPVApplication.h"
#include "vtkPVConfig.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkSMDataObjectDisplayProxy.h"

#include <stdarg.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVSourcesNavigationWindow );
vtkCxxRevisionMacro(vtkPVSourcesNavigationWindow, "1.34");

//-----------------------------------------------------------------------------
vtkPVSourcesNavigationWindow::vtkPVSourcesNavigationWindow()
{
  this->Width     = -1;
  this->Height    = -1;
  this->Canvas    = vtkKWCanvas::New();
  this->ScrollBar = vtkKWScrollbar::New();
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
void vtkPVSourcesNavigationWindow::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

  const char *wname = this->GetWidgetName();
  
  this->Canvas->SetParent(this);
  this->Canvas->Create(); 
  this->Canvas->SetHighlightThickness(0);
  this->Canvas->SetBackgroundColor(1.0, 1.0, 1.0);

  if (this->Width > 0)
    {
    this->Canvas->SetWidth(this->Width);
    }
  if (this->Height > 0)
    {
    this->Canvas->SetHeight(this->Height);
    }

  ostrstream command;
  this->ScrollBar->SetParent(this);
  command << this->Canvas->GetWidgetName()
          << " yview" << ends;
  char* commandStr = command.str();
  this->ScrollBar->Create();
  this->ScrollBar->SetConfigurationOption("-command", commandStr);
  delete[] commandStr;

  this->Script("%s configure -yscrollcommand \"%s set\"", 
               this->Canvas->GetWidgetName(),
               this->ScrollBar->GetWidgetName());

  this->Canvas->AddBinding("<Configure>", this, "Reconfigure");

  this->Script("grid %s -row 0 -column 0 -sticky news", 
               this->Canvas->GetWidgetName());
  this->Script("grid columnconfig %s 0 -weight 1", wname);
  this->Script("grid rowconfig %s 0 -weight 1", wname);
  this->PopupMenu->SetParent(this);
  this->PopupMenu->Create();
  this->PopupMenu->SetTearOff(0);

  int index;
  index = this->PopupMenu->AddCommand("Delete", this, "PopupDeleteCallback");
  this->PopupMenu->SetItemUnderline(index, 0);
  this->PopupMenu->SetItemHelpString(
    index, 
    "Delete the module.  Module that are used by filters cannot be deleted.");

  char *var = this->PopupMenu->CreateItemVariableName(this, "Visibility");

  index = this->PopupMenu->AddCheckButton(
    "Visibility", this, "PopupVisibilityCallback");  
  this->PopupMenu->SetItemVariable(index, var);
  this->PopupMenu->SetItemUnderline(index, 0);
  this->PopupMenu->SetItemHelpString(
    index, "Set the visibility for this module.");

  delete [] var;

  // Representation

  this->PopupMenu->AddSeparator();
  var = this->PopupMenu->CreateItemVariableName(this, "Representation");

  index = this->PopupMenu->AddRadioButton(
    "Outline", this, "PopupOutlineRepresentationCallback");
  this->PopupMenu->SetItemVariable(index, var);
  this->PopupMenu->SetItemSelectedValueAsInt(
    index, vtkSMDataObjectDisplayProxy::OUTLINE);
  this->PopupMenu->SetItemHelpString(
    index, "Outline is edges of the bounding box.");

  index = this->PopupMenu->AddRadioButton(
    "Surface", this, "PopupSurfaceRepresentationCallback");
  this->PopupMenu->SetItemVariable(index, var);
  this->PopupMenu->SetItemSelectedValueAsInt(
    index, vtkSMDataObjectDisplayProxy::SURFACE);
  this->PopupMenu->SetItemHelpString(
    index, "Only external (non shared) faces of cells are displayed.");

  index = this->PopupMenu->AddRadioButton(
    "Wireframe of Surface", this, "PopupWireframeRepresentationCallback");
  this->PopupMenu->SetItemVariable(index, var);
  this->PopupMenu->SetItemSelectedValueAsInt(
    index, vtkSMDataObjectDisplayProxy::WIREFRAME);
  this->PopupMenu->SetItemHelpString(
    index, "Wirefrace of surface (non shared) faces.");

  index = this->PopupMenu->AddRadioButton(
    "Points of Surface", this, "PopupPointsRepresentationCallback");
  this->PopupMenu->SetItemVariable(index, var);
  this->PopupMenu->SetItemSelectedValueAsInt(
    index, vtkSMDataObjectDisplayProxy::POINTS);
  this->PopupMenu->SetItemHelpString(
    index, "Points of surface (non shared) faces.");

  delete [] var;

  // Interpolation

  this->PopupMenu->AddSeparator();

  var = this->PopupMenu->CreateItemVariableName(this, "Interpolation");

  index = this->PopupMenu->AddRadioButton(
    "Flat", this, "PopupFlatInterpolationCallback");
  this->PopupMenu->SetItemVariable(index, var);
  this->PopupMenu->SetItemSelectedValueAsInt(
    index, vtkSMDataObjectDisplayProxy::FLAT);
  this->PopupMenu->SetItemHelpString(
    index, "Flat shading makes the surfaace look faceted.");

  index = this->PopupMenu->AddRadioButton(
    "Gouraud", this, "PopupGouraudInterpolationCallback");
  this->PopupMenu->SetItemVariable(index, var);
  this->PopupMenu->SetItemSelectedValueAsInt(
    index, vtkSMDataObjectDisplayProxy::GOURAND);
  this->PopupMenu->SetItemHelpString(
    index, "When the data has normals, Gouraud shading make the surface look smooth.");

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

  if ( module->IsDeletable())
    {
    this->PopupMenu->SetItemStateToNormal("Delete");
    }
  else
    {
    this->PopupMenu->SetItemStateToDisabled("Delete");
    }

  char *rbv = this->PopupMenu->CreateItemVariableName(this, "Visibility");
  this->PopupMenu->SetItemVariableValueAsInt(rbv, module->GetVisibility());
  delete [] rbv;

  rbv = this->PopupMenu->CreateItemVariableName(this, "Interpolation");
  this->PopupMenu->SetItemVariableValueAsInt(
    rbv, module->GetDisplayProxy()->GetInterpolationCM());
  delete [] rbv;

  // Set the value of the representation radio button.

  rbv = this->PopupMenu->CreateItemVariableName(this, "Representation");
  this->PopupMenu->SetItemVariableValueAsInt(
    rbv, module->GetDisplayProxy()->GetRepresentationCM());
  delete [] rbv;

  // Show the popup menu in correct location (x, y is cursor position).
  this->PopupMenu->PopUp(x, y);
}

//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupDeleteCallback()
{
  this->PopupModule->DeleteCallback();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupVisibilityCallback()
{
  char *rbv = this->PopupMenu->CreateItemVariableName(this, "Visibility");
  if (this->PopupMenu->GetItemVariableValueAsInt(rbv))
    {
    this->PopupModule->SetVisibility(1);
    }
  else
    {
    this->PopupModule->SetVisibility(0);
    }
  delete [] rbv;
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupFlatInterpolationCallback()
{
  this->PopupModule->GetDisplayProxy()->SetInterpolationCM(
    vtkSMDataObjectDisplayProxy::FLAT);
  this->PopupModule->UpdateProperties(); 
    // so that DisplayGUI also shows
   // the correect interpolation/representation.
  this->PopupModule->GetPVRenderView()->EventuallyRender();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupGouraudInterpolationCallback()
{
  this->PopupModule->GetDisplayProxy()->SetInterpolationCM(
    vtkSMDataObjectDisplayProxy::GOURAND);
  this->PopupModule->UpdateProperties();
    // so that DisplayGUI also shows
   // the correect interpolation/representation.
  this->PopupModule->GetPVRenderView()->EventuallyRender();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupOutlineRepresentationCallback()
{
  this->PopupModule->GetDisplayProxy()->SetRepresentationCM(
    vtkSMDataObjectDisplayProxy::OUTLINE);
  this->PopupModule->UpdateProperties();
    // so that DisplayGUI also shows
   // the correect interpolation/representation.
  this->PopupModule->GetPVRenderView()->EventuallyRender();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupSurfaceRepresentationCallback()
{
  this->PopupModule->GetDisplayProxy()->SetRepresentationCM(
    vtkSMDataObjectDisplayProxy::SURFACE);
  this->PopupModule->UpdateProperties();
    // so that DisplayGUI also shows
   // the correect interpolation/representation.
  this->PopupModule->GetPVRenderView()->EventuallyRender();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupWireframeRepresentationCallback()
{
  this->PopupModule->GetDisplayProxy()->SetRepresentationCM(
    vtkSMDataObjectDisplayProxy::WIREFRAME);
  this->PopupModule->UpdateProperties();
    // so that DisplayGUI also shows
   // the correect interpolation/representation.
  this->PopupModule->GetPVRenderView()->EventuallyRender();
}
//-----------------------------------------------------------------------------
void vtkPVSourcesNavigationWindow::PopupPointsRepresentationCallback()
{
  this->PopupModule->GetDisplayProxy()->SetRepresentationCM(
    vtkSMDataObjectDisplayProxy::POINTS);
  this->PopupModule->UpdateProperties();
    // so that DisplayGUI also shows
   // the correect interpolation/representation.
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
