/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWListBox.cxx
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
#include "vtkKWListBox.h"
#include "vtkObjectFactory.h"


//------------------------------------------------------------------------------
vtkKWListBox* vtkKWListBox::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkKWListBox");
  if(ret)
    {
    return (vtkKWListBox*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKWListBox;
}


int vtkKWListBoxCommand(ClientData cd, Tcl_Interp *interp,
			int argc, char *argv[]);

vtkKWListBox::vtkKWListBox()
{   
  this->CurrentSelection = 0;
  this->Item = 0; 
  this->CommandFunction = vtkKWListBoxCommand;
}

vtkKWListBox::~vtkKWListBox()
{
  delete [] this->Item;
  delete [] this->CurrentSelection;
}


int vtkKWListBox::GetNumberOfItems()
{
  this->Script("%s.list size", this->GetWidgetName());
  char* result = this->Application->GetMainInterp()->result;
  return atoi(result);
}

void vtkKWListBox::DeleteRange(int start, int end)
{
  this->Script("%s.list delete %d %d", this->GetWidgetName(), start, end);
}

const char* vtkKWListBox::GetItem(int index)
{
  this->Script("%s.list get %d", this->GetWidgetName(), index);
  char* result = this->Application->GetMainInterp()->result;
  delete [] this->Item;
  this->Item = strcpy(new char[strlen(result)+1], result);
  return this->Item;
}

int vtkKWListBox::GetSelectionIndex()
{
  this->Script("%s.list curselection", this->GetWidgetName(),
	       this->GetWidgetName());
  char* result = this->Application->GetMainInterp()->result;
  return atoi(result);
}

  
const char *vtkKWListBox::GetSelection()
{
  this->Script("%s.list get [%s.list curselection]", this->GetWidgetName(),
	       this->GetWidgetName());
  char* result = this->Application->GetMainInterp()->result;
  this->CurrentSelection = strcpy(new char[strlen(result)+1], result);
  return this->CurrentSelection;
}


void vtkKWListBox::InsertEntry(int index, const char *name)
{
  this->Script("%s.list insert %d {%s}", this->GetWidgetName(), index, name);
}


 
void vtkKWListBox::SetDoubleClickCallback(vtkKWObject* obj, 
					  const char* methodAndArgs)
{
  this->Script("bind %s.list <Double-1> {%s %s}", this->GetWidgetName(),
	       obj->GetTclName(), methodAndArgs);
}


int vtkKWListBox::AppendUnique(const char* name)
{
  int size = this->GetNumberOfItems();
  int found = 0;
  for(int i =0; i < size; i++)
    {
    if(strcmp(this->GetItem(i), name) == 0)
      {
      found = 1;
      break;
      }
    }
  if(!found)
    {
    this->InsertEntry(size, name);
    }
  return !found;
}



void vtkKWListBox::Create(vtkKWApplication *app, char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("OptionListBox already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  
  this->Script("frame %s ", wname);
  this->Script("pack %s", wname);
  this->Script("scrollbar %s.scroll -command \"%s.list yview\"", 
	       wname, wname);
  this->Script("listbox %s.list  -yscroll \"%s.scroll set\" -setgrid 1 %s", 
	       wname, wname, args);
  this->Script("pack %s.scroll -side right -fill y", wname);
  this->Script("pack %s.list -side left -expand 1 -fill both", wname);
}


void vtkKWListBox::SetWidth(int w)
{
  this->Script("%s.list configure -width %d", this->GetWidgetName(), w);
}

void vtkKWListBox::SetHeight(int h)
{
  this->Script("%s.list configure -height %d", this->GetWidgetName(), h);
}

void vtkKWListBox::DeleteAll()
{
  int n =  this->GetNumberOfItems();
  this->DeleteRange(0, n);
}
