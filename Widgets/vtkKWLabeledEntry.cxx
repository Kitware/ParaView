/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLabeledEntry.cxx
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
#include "vtkKWApplication.h"
#include "vtkKWLabeledEntry.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkKWLabeledEntry* vtkKWLabeledEntry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWLabeledEntry");
  if(ret)
    {
    return (vtkKWLabeledEntry*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWLabeledEntry;
}

int vtkKWLabeledEntryCommand(ClientData cd, Tcl_Interp *interp,
			     int argc, char *argv[]);

vtkKWLabeledEntry::vtkKWLabeledEntry()
{
  this->CommandFunction = vtkKWLabeledEntryCommand;

  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->Entry = vtkKWEntry::New();
  this->Entry->SetParent(this);
}

vtkKWLabeledEntry::~vtkKWLabeledEntry()
{
  this->Label->Delete();
  this->Label = NULL;
  this->Entry->Delete();
  this->Entry = NULL;
}

void vtkKWLabeledEntry::SetLabel(const char *text)
{
  this->Script("%s configure -text {%s}",
               this->Label->GetWidgetName(), text);  
}

void vtkKWLabeledEntry::Create(vtkKWApplication *app)
{
  const char *wname;
  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("LabeledEntry already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat",wname);

  this->Label->Create(app, "");
  this->Entry->Create(app, "");

  this->Script("pack %s %s -side left", this->Label->GetWidgetName(),
	       this->Entry->GetWidgetName());
}

void vtkKWLabeledEntry::SetValue(const char *value)
{
  this->Entry->SetValue(value);
}

void vtkKWLabeledEntry::SetValue(int a)
{
  this->Entry->SetValue(a);
}

void vtkKWLabeledEntry::SetValue(float f,int size)
{
  this->Entry->SetValue(f, size);
}

char *vtkKWLabeledEntry::GetValue()
{
  return this->Entry->GetValue();
}

int vtkKWLabeledEntry::GetValueAsInt()
{
  return this->Entry->GetValueAsInt();
}

float vtkKWLabeledEntry::GetValueAsFloat()
{
  return this->Entry->GetValueAsFloat();
}
