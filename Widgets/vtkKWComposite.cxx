/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWComposite.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkKWView.h"
#include "vtkKWApplication.h"

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

void vtkKWComposite::CreateProperties()
{
  vtkKWApplication *app = this->Application;

  // if we have a view then use its attachment point
  if (this->View && this->View->GetPropertiesParent())
    {
    this->Notebook->SetParent(this->View->GetPropertiesParent());
    this->Notebook2->SetParent(this->View->GetPropertiesParent());
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
    this->Notebook->SetParent(this->TopLevel);
    this->Notebook2->SetParent(this->TopLevel);
    }
  this->Notebook->Create(this->Application,"");
  this->Notebook2->Create(this->Application,"");

  this->Script("pack %s -pady 2 -padx 2 -fill both -expand yes -anchor n",
               this->Notebook->GetWidgetName());
}

void vtkKWComposite::Select(vtkKWView *v)
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
