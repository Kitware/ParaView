/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVBoundsDisplay.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVBoundsDisplay.h"

#include "vtkArrayMap.txx"
#include "vtkKWApplication.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVData.h"
#include "vtkPVInputMenu.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

vtkCxxSetObjectMacro(vtkPVBoundsDisplay, Widget, vtkKWBoundsDisplay);
vtkCxxSetObjectMacro(vtkPVBoundsDisplay, InputMenu, vtkPVInputMenu);

//----------------------------------------------------------------------------
vtkPVBoundsDisplay* vtkPVBoundsDisplay::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVBoundsDisplay");
  if(ret)
    {
    return (vtkPVBoundsDisplay*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVBoundsDisplay;
}

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
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("BoundsDisplay already created");
    return;
    }
  this->SetApplication(app);

  this->Script("frame %s", this->GetWidgetName());
  this->Widget->SetParent(this);
  this->Widget->SetShowHideFrame( this->GetShowHideFrame() );
  this->Widget->Create(app);
  if (this->FrameLabel)
    {
    this->Widget->SetLabel(this->FrameLabel);
    }
  this->Script("pack %s -side top -expand t -fill x", 
               this->Widget->GetWidgetName());
}

void vtkPVBoundsDisplay::SetLabel(const char* label)
{
  this->SetFrameLabel(label);
  if (this->Application && this->FrameLabel)
    {
    this->Widget->SetLabel(this->FrameLabel);
    }
}

const char* vtkPVBoundsDisplay::GetLabel()
{
  return this->GetFrameLabel();
}


//----------------------------------------------------------------------------
void vtkPVBoundsDisplay::Update()
{
  vtkPVData *input;
  float bds[6];

  if (this->InputMenu == NULL)
    {
    vtkErrorMacro("Input menu has not been set.");
    return;
    }

  input = this->InputMenu->GetCurrentValue()->GetPVOutput();
  if (input == NULL)
    {
    bds[0] = bds[2] = bds[4] = VTK_LARGE_FLOAT;
    bds[1] = bds[3] = bds[5] = -VTK_LARGE_FLOAT;
    }
  else
    {
    input->GetBounds(bds);
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

vtkPVBoundsDisplay* vtkPVBoundsDisplay::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVBoundsDisplay::SafeDownCast(clone);
}

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

void vtkPVBoundsDisplay::CopyProperties(vtkPVWidget* clone, 
					vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVBoundsDisplay* pvbd = vtkPVBoundsDisplay::SafeDownCast(clone);
  if (pvbd)
    {
    pvbd->SetShowHideFrame(this->GetShowHideFrame());
    pvbd->SetFrameLabel(this->GetFrameLabel());
    pvbd->SetTraceName(this->GetFrameLabel());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVBoundsDisplay.");
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
