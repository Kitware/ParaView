/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWListBox.cxx
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
  
  this->Scrollbar = vtkKWWidget::New();
  this->Scrollbar->SetParent(this);
  
  this->Listbox = vtkKWWidget::New();
  this->Listbox->SetParent(this);
}

vtkKWListBox::~vtkKWListBox()
{
  delete [] this->Item;
  delete [] this->CurrentSelection;
  
  this->Scrollbar->Delete();
  this->Listbox->Delete();
  
}


int vtkKWListBox::GetNumberOfItems()
{
  this->Script("%s size", this->Listbox->GetWidgetName());
  char* result = this->Application->GetMainInterp()->result;
  return atoi(result);
}

void vtkKWListBox::DeleteRange(int start, int end)
{
  this->Script("%s delete %d %d", this->Listbox->GetWidgetName(), start, end);
}

const char* vtkKWListBox::GetItem(int index)
{
  this->Script("%s get %d", this->Listbox->GetWidgetName(), index);
  char* result = this->Application->GetMainInterp()->result;
  delete [] this->Item;
  this->Item = strcpy(new char[strlen(result)+1], result);
  return this->Item;
}

int vtkKWListBox::GetSelectionIndex()
{
  this->Script("%s curselection", this->Listbox->GetWidgetName(),
	       this->GetWidgetName());
  char* result = this->Application->GetMainInterp()->result;
  return atoi(result);
}

  
const char *vtkKWListBox::GetSelection()
{
  this->Script("%s get [%s curselection]", this->Listbox->GetWidgetName(),
	       this->Listbox->GetWidgetName());
  char* result = this->Application->GetMainInterp()->result;
  this->CurrentSelection = strcpy(new char[strlen(result)+1], result);
  return this->CurrentSelection;
}


void vtkKWListBox::InsertEntry(int index, const char *name)
{
  this->Script("%s insert %d {%s}", this->Listbox->GetWidgetName(), index, name);
}


 
void vtkKWListBox::SetDoubleClickCallback(vtkKWObject* obj, 
					  const char* methodAndArgs)
{
  this->Script("bind %s <Double-1> {%s %s}", this->Listbox->GetWidgetName(),
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



void vtkKWListBox::Create(vtkKWApplication *app, const char *args)
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
  this->Scrollbar->Create( app, "scrollbar", "" );
  
  this->Listbox->Create( app, "listbox", args );
  
  this->Script( "%s configure -yscroll {%s set}", 
                this->Listbox->GetWidgetName(),
                this->Scrollbar->GetWidgetName());
  
  this->Script( "%s configure -command {%s yview}", 
                this->Scrollbar->GetWidgetName(),
                this->Listbox->GetWidgetName());
  
  this->Script("pack %s -side right -fill y", this->Scrollbar->GetWidgetName());
  this->Script("pack %s -side left -expand 1 -fill both", this->Listbox->GetWidgetName());
}


void vtkKWListBox::SetWidth(int w)
{
  this->Script("%s configure -width %d", this->Listbox->GetWidgetName(), w);
}

void vtkKWListBox::SetHeight(int h)
{
  this->Script("%s configure -height %d", this->Listbox->GetWidgetName(), h);
}

void vtkKWListBox::DeleteAll()
{
  int n =  this->GetNumberOfItems();
  this->DeleteRange(0, n);
}

void vtkKWListBox::SetBalloonHelpString(const char *str)
{
  this->Listbox->SetBalloonHelpString( str );
  this->Scrollbar->SetBalloonHelpString( str );
}


void vtkKWListBox::SetBalloonHelpJustification( int j )
{
  this->Listbox->SetBalloonHelpJustification( j );
  this->Scrollbar->SetBalloonHelpJustification( j );
}
