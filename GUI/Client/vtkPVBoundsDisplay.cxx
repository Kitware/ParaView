/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBoundsDisplay.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVBoundsDisplay.h"

#include "vtkArrayMap.txx"
#include "vtkKWApplication.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVBoundsDisplay);
vtkCxxRevisionMacro(vtkPVBoundsDisplay, "1.19");

vtkCxxSetObjectMacro(vtkPVBoundsDisplay, Widget, vtkKWBoundsDisplay);
vtkCxxSetObjectMacro(vtkPVBoundsDisplay, InputMenu, vtkPVInputMenu);

//----------------------------------------------------------------------------
int vtkPVBoundsDisplayCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVBoundsDisplay::vtkPVBoundsDisplay()
{
  this->CommandFunction = vtkPVBoundsDisplayCommand;

  this->Widget = vtkKWBoundsDisplay::New();
  this->InputMenu = 0;
  this->ShowHideFrame = 0;
  this->FrameLabel = 0;
}

//----------------------------------------------------------------------------
vtkPVBoundsDisplay::~vtkPVBoundsDisplay()
{
  this->Widget->Delete();
  this->Widget = NULL;
  this->SetInputMenu(NULL);
  this->SetFrameLabel(0);
}


//----------------------------------------------------------------------------
void vtkPVBoundsDisplay::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("BoundsDisplay already created");
    return;
    }
  this->SetApplication(app);

  this->Script("frame %s", this->GetWidgetName());
  this->Widget->SetParent(this);
  this->Widget->SetShowHideFrame( this->GetShowHideFrame() );
  this->Widget->Create(app, "");
  if (this->FrameLabel)
    {
    this->Widget->SetLabel(this->FrameLabel);
    }
  this->Script("pack %s -side top -expand t -fill x", 
               this->Widget->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVBoundsDisplay::SetLabel(const char* label)
{
  this->SetFrameLabel(label);
  if (this->GetApplication() && this->FrameLabel)
    {
    this->Widget->SetLabel(this->FrameLabel);
    }
}

//----------------------------------------------------------------------------
const char* vtkPVBoundsDisplay::GetLabel()
{
  return this->GetFrameLabel();
}


//----------------------------------------------------------------------------
void vtkPVBoundsDisplay::Update()
{
  this->Superclass::Update();

  vtkPVSource *input;
  double bds[6];

  if (this->InputMenu == NULL)
    {
    vtkErrorMacro("Input menu has not been set.");
    return;
    }

  input = this->InputMenu->GetCurrentValue();
  if (input == NULL)
    {
    bds[0] = bds[2] = bds[4] = VTK_LARGE_FLOAT;
    bds[1] = bds[3] = bds[5] = -VTK_LARGE_FLOAT;
    }
  else
    {
    input->GetDataInformation()->GetBounds(bds);
    }

  this->Widget->SetBounds(bds);
}

//----------------------------------------------------------------------------
void vtkPVBoundsDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputMenu: " << this->GetInputMenu();
  os << indent << "ShowHideFrame: " << this->GetShowHideFrame();
  os << indent << "Widget: " << this->GetWidget();
}

//----------------------------------------------------------------------------
vtkPVBoundsDisplay* vtkPVBoundsDisplay::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVBoundsDisplay::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVBoundsDisplay::ClonePrototypeInternal(vtkPVSource* pvSource,
                                vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* pvWidget = 0;
  // Check if a clone of this widget has already been created
  if ( map->GetItem(this, pvWidget) != VTK_OK )
    {
    // If not, create one and add it to the map
    pvWidget = this->NewInstance();
    map->SetItem(this, pvWidget);
    // Now copy all the properties
    this->CopyProperties(pvWidget, pvSource, map);

    vtkPVBoundsDisplay* pvBounds = vtkPVBoundsDisplay::SafeDownCast(pvWidget);
    if (!pvBounds)
      {
      vtkErrorMacro("Internal error. Could not downcast pointer.");
      pvWidget->Delete();
      return 0;
      }
    
    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      pvBounds->SetInputMenu(im);
      im->Delete();
      }
    }
  else
    {
    // Increment the reference count. This is necessary
    // to make the behavior same whether a widget is created
    // or returned from the map. Always call Delete() after
    // cloning.
    pvWidget->Register(this);
    }


  // note pvSelect == pvWidget
  return pvWidget;
}

//----------------------------------------------------------------------------
void vtkPVBoundsDisplay::CopyProperties(
  vtkPVWidget* clone, vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVBoundsDisplay* pvbd = vtkPVBoundsDisplay::SafeDownCast(clone);
  if (pvbd)
    {
    pvbd->SetShowHideFrame(this->GetShowHideFrame());
    const char* frameLabel = this->GetFrameLabel();
    pvbd->SetFrameLabel(frameLabel);
    if (frameLabel && frameLabel[0] &&
        (pvbd->TraceNameState == vtkPVWidget::Uninitialized ||
         pvbd->TraceNameState == vtkPVWidget::Default) )
      {
      pvbd->SetTraceName(frameLabel);
      }
    }
  else 
    {
    vtkErrorMacro(
      "Internal error. Could not downcast clone to PVBoundsDisplay.");
    }
}

//----------------------------------------------------------------------------
int vtkPVBoundsDisplay::ReadXMLAttributes(vtkPVXMLElement* element,
                                          vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  if(!element->GetScalarAttribute("show_hide_frame", &this->ShowHideFrame))
    {
    this->ShowHideFrame = 0;
    }
  
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->SetFrameLabel(label);
    }

  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if(!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }
  
  vtkPVXMLElement* ime = element->LookupElement(input_menu);
  if (!ime)
    {
    vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
    return 0;
    }
  
  vtkPVWidget* w = this->GetPVWidgetFromParser(ime, parser);
  vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast(w);
  if(!imw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
    return 0;
    }
  imw->AddDependent(this);
  this->SetInputMenu(imw);
  imw->Delete();
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVBoundsDisplay::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Widget);
  this->PropagateEnableState(this->InputMenu);
}


