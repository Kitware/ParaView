/*=========================================================================

  Module:    vtkKWComposite.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWComposite.h"

#include "vtkKWApplication.h"
#include "vtkKWMenu.h"
#include "vtkKWNotebook.h"
#include "vtkKWView.h"
#include "vtkKWWindow.h"

vtkCxxRevisionMacro(vtkKWComposite, "1.1");

int vtkKWCompositeCommand(ClientData cd, Tcl_Interp *interp,
                          int argc, char *argv[]);

vtkKWComposite::vtkKWComposite()
{
  this->CommandFunction = vtkKWCompositeCommand;
  this->Notebook = vtkKWNotebook::New();
  this->Notebook2 = vtkKWNotebook::New();
  this->PropertiesCreated = 0;
  this->TopLevel = NULL;
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
  if (!this->GetApplication())
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
  vtkKWApplication *app = this->GetApplication();

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
  this->Notebook->Create(app, "");
  this->Notebook2->Create(app, "");

  // I do not think this should be here, but removing it will probably break VolView.
  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
}

void vtkKWComposite::Deselect(vtkKWView *v)
{
  this->LastSelectedProperty = 
    v->GetParentWindow()->GetMenuView()->GetRadioButtonValue(
      v->GetParentWindow()->GetMenuView(),"Radio");
}

void vtkKWComposite::Select(vtkKWView* /*v*/)
{
  // make sure we have an applicaiton
  if (!this->GetApplication())
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
  this->ExtractRevision(os,"$Revision: 1.1 $");
}

//----------------------------------------------------------------------------
#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef GetProp
// Define possible mangled names.
vtkProp* vtkKWComposite::GetPropA()
{
  return this->GetPropInternal();
}
vtkProp* vtkKWComposite::GetPropW()
{
  return this->GetPropInternal();
}
#endif
vtkProp* vtkKWComposite::GetProp()
{
  return this->GetPropInternal();
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

