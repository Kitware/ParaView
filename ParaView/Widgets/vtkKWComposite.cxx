/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWComposite.cxx
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

#include "vtkKWComposite.h"
#include "vtkKWView.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkKWMenu.h"

int vtkKWCompositeCommand(ClientData cd, Tcl_Interp *interp,
			  int argc, char *argv[]);

vtkKWComposite::vtkKWComposite()
{
  this->CommandFunction = vtkKWCompositeCommand;
  this->Notebook = vtkKWNotebook::New();
  this->Notebook2 = vtkKWNotebook::New();
  this->PropertiesCreated = 0;
  this->TopLevel = NULL;
  this->Application = NULL;
  this->View = NULL;
  this->LastSelectedProperty = -1;
  
  this->PropertiesParent = NULL;
}

vtkKWComposite::~vtkKWComposite()
{
  this->Notebook->SetParent(NULL);
  this->Notebook->Delete();
  this->Notebook2->SetParent(NULL);
  this->Notebook2->Delete();
  if (this->TopLevel)
    {
    this->TopLevel->UnRegister(this);
    this->TopLevel = NULL;
    }
  if (this->Application)
    {
    this->Application->UnRegister(this);
    this->Application = NULL;
    }
  if (this->View)
    {
    this->View->UnRegister(this);
    this->View = NULL;
    }
  
  if (this->PropertiesParent)
    {
    this->PropertiesParent->UnRegister(this);
    this->PropertiesParent = NULL;
    }
}

void vtkKWComposite::MakeSelected()
{
  if (this->View)
    {
    this->View->SetSelectedComposite(this);
    }
}

void vtkKWComposite::SetView(vtkKWView *_arg)
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting View to " << _arg ); 
  if (this->View != _arg) 
    { 
    if (this->View != NULL) { this->View->UnRegister(this); }
    this->View = _arg; 
    if (this->View != NULL) { this->View->Register(this); } 
    this->Modified(); 
    } 
} 

void vtkKWComposite::InitializeProperties()
{
  // make sure we have an applicaiton
  if (!this->Application)
    {
    if (this->View)
      {
      this->SetApplication(this->View->GetApplication());
      }
    else
      {
      vtkErrorMacro("attempt to update properties without an application set");
      }
    }
  
  // do we need to create the props ?
  if (!this->PropertiesCreated)
    {
    this->CreateProperties();
    this->PropertiesCreated = 1;
    }
}


void vtkKWComposite::SetPropertiesParent(vtkKWWidget *parent)
{
  if (this->PropertiesParent == parent)
    {
    return;
    }
  if (this->PropertiesParent)
    {
    vtkErrorMacro("Cannot reparent properties.");
    return;
    }
  this->PropertiesParent = parent;
  parent->Register(this);
}


void vtkKWComposite::CreateProperties()
{
  vtkKWApplication *app = this->Application;

  // If the user has not set the properties parent.
  if (this->PropertiesParent == NULL)
    {
    if (this->View && this->View->GetPropertiesParent())
      { // if we have a view then use its attachment point
      this->SetPropertiesParent(this->View->GetPropertiesParent());
      }
    else
      {
      // create and use a toplevel shell
      this->TopLevel = vtkKWWidget::New();
      this->TopLevel->Create(app,"toplevel","");
      this->Script("wm title %s \"Properties\"",
		   this->TopLevel->GetWidgetName());
      this->Script("wm iconname %s \"Properties\"",
                 this->TopLevel->GetWidgetName());
      this->SetPropertiesParent(this->TopLevel);
      }
    }

  this->Notebook->SetParent(this->PropertiesParent);
  this->Notebook2->SetParent(this->PropertiesParent);
  this->Notebook->Create(this->Application,"");
  this->Notebook2->Create(this->Application,"");

  // I do not think this should be here, but removing it will probably break VolView.
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
}

void vtkKWComposite::Deselect(vtkKWView *v)
{
  if ( v->GetParentWindow()->GetUseMenuProperties() )
    {
    this->LastSelectedProperty = 
      v->GetParentWindow()->GetMenuProperties()->GetRadioButtonValue(
	v->GetParentWindow()->GetMenuProperties(),"Radio");
    }
}

void vtkKWComposite::Select(vtkKWView* /*v*/)
{
  // make sure we have an applicaiton
  if (!this->Application)
    {
    if (this->View)
      {
      this->SetApplication(this->View->GetApplication());
      }
    else
      {
      vtkErrorMacro("attempt to select composite without an application set");
      }
    }
}

void vtkKWComposite::SerializeRevision(ostream& os, vtkIndent indent)
{
  vtkKWObject::SerializeRevision(os,indent);
  os << indent << "vtkKWComposite ";
  this->ExtractRevision(os,"$Revision: 1.15 $");
}

//----------------------------------------------------------------------------
void vtkKWComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Notebook2: " << this->GetNotebook2() << endl;
  os << indent << "Notebook: " << this->GetNotebook() << endl;
  os << indent << "PropertiesParent: " << this->GetPropertiesParent() << endl;
  os << indent << "View: " << this->GetView() << endl;
}
