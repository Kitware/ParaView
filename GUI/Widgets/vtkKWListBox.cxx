/*=========================================================================

  Module:    vtkKWListBox.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWListBox.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWListBox);
vtkCxxRevisionMacro(vtkKWListBox, "1.32");


//----------------------------------------------------------------------------
int vtkKWListBoxCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWListBox::vtkKWListBox()
{   
  this->CurrentSelection = 0;
  this->Item = 0; 
  this->CommandFunction = vtkKWListBoxCommand;
  
  this->Scrollbar = vtkKWWidget::New();
  this->Scrollbar->SetParent(this);
  
  this->Listbox = vtkKWWidget::New();
  this->Listbox->SetParent(this);

  this->ScrollbarFlag = 1;
}

//----------------------------------------------------------------------------
vtkKWListBox::~vtkKWListBox()
{
  delete [] this->Item;
  delete [] this->CurrentSelection;
  
  this->Scrollbar->Delete();
  this->Listbox->Delete();
  
}


//----------------------------------------------------------------------------
int vtkKWListBox::GetNumberOfItems()
{
  if ( !this->IsCreated() )
    {
    return 0;
    }
  return atoi(this->Script("%s size", this->Listbox->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWListBox::DeleteRange(int start, int end)
{
  int enabled = this->Enabled;
  this->SetEnabled(1);
  this->Script("%s delete %d %d", this->Listbox->GetWidgetName(), start, end);
  this->SetEnabled(enabled);
}

//----------------------------------------------------------------------------
const char* vtkKWListBox::GetItem(int index)
{
  const char* result = 
    this->Script("%s get %d", this->Listbox->GetWidgetName(), index);
  delete [] this->Item;
  this->Item = strcpy(new char[strlen(result) + 1], result);
  return this->Item;
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetSelectionIndex(int sel)
{
  if ( sel < 0 )
    {
    return;
    }
  this->Script("%s selection set %d", this->Listbox->GetWidgetName(), sel);
}

//----------------------------------------------------------------------------
int vtkKWListBox::GetSelectionIndex()
{
  if ( !this->IsCreated() )
    {
    return 0;
    }
  const char* result = this->Script(
    "%s curselection", this->Listbox->GetWidgetName(), this->GetWidgetName());
  if ( strlen(result)>0 )
    {
    return atoi(result);
    }
  return -1;
}

  
//----------------------------------------------------------------------------
const char *vtkKWListBox::GetSelection()
{
  if ( !this->IsCreated() )
    {
    return 0;
    }
  if ( this->GetSelectionIndex() < 0 )
    {
    return 0;
    }
  const char* result = this->Script(
    "%s get [%s curselection]", 
    this->Listbox->GetWidgetName(),
    this->Listbox->GetWidgetName());

  if (this->CurrentSelection)
    {
    delete [] this->CurrentSelection;
    }
  this->CurrentSelection = strcpy(new char[strlen(result)+1], result);
  return this->CurrentSelection;
}


//----------------------------------------------------------------------------
void vtkKWListBox::SetSelectState(int idx, int state)
{
  if ( idx < 0 )
    {
    return;
    }

  int was_disabled = !this->Enabled;
  if (was_disabled)
    {
    this->SetEnabled(1);
    }
  if (state)
    {
    this->Script("%s selection set %d", this->Listbox->GetWidgetName(), idx);
    }
  else
    {
    this->Script("%s selection clear %d", this->Listbox->GetWidgetName(), idx);
    }
  if (was_disabled)
    {
    this->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
int vtkKWListBox::GetItemIndex(const char* item)
{
  if ( !item )
    {
    return 0;
    }
  int cc;
  for ( cc = 0; cc < this->GetNumberOfItems(); cc ++ )
    {
    if ( strcmp(item, this->GetItem(cc)) == 0 )
      {
      return cc;
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkKWListBox::GetSelectState(int idx)
{
  if ( idx < 0 )
    {
    return 0;
    }
  return atoi(this->Script("%s selection includes %d", 
                           this->Listbox->GetWidgetName(), idx));

}

//----------------------------------------------------------------------------
void vtkKWListBox::InsertEntry(int index, const char *name)
{
  if ( !this->IsCreated() )
    {
    return;
    }
  int enabled = this->Enabled;
  if ( !enabled )
    {
    this->SetEnabled(1);
    }
  this->Script("%s insert %d {%s}", this->Listbox->GetWidgetName(), index, name);
  if ( !enabled )
    {
    this->SetEnabled(0);
    }
}


 
//----------------------------------------------------------------------------
void vtkKWListBox::SetDoubleClickCallback(vtkKWObject* obj, 
                                          const char* methodAndArgs)
{
  this->Script("bind %s <Double-1> {%s %s}", this->Listbox->GetWidgetName(),
               obj->GetTclName(), methodAndArgs);
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetSingleClickCallback(vtkKWObject* obj, 
                                          const char* methodAndArgs)
{
  this->Script("bind %s <ButtonRelease-1> {%s %s}", this->Listbox->GetWidgetName(),
               obj->GetTclName(), methodAndArgs);
}


//----------------------------------------------------------------------------
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



//----------------------------------------------------------------------------
void vtkKWListBox::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Scrollbar->Create(app, "scrollbar", NULL);
  
  this->Listbox->Create(app, "listbox", args);
  
  this->Script("%s configure -yscroll {%s set}", 
               this->Listbox->GetWidgetName(),
               this->Scrollbar->GetWidgetName());
  
  this->Script("%s configure -command {%s yview}", 
               this->Scrollbar->GetWidgetName(),
               this->Listbox->GetWidgetName());
    
  if (this->ScrollbarFlag)
    {
    this->Script("pack %s -side right -fill y", 
                 this->Scrollbar->GetWidgetName());
    }
  this->Script("pack %s -side left -expand 1 -fill both", 
               this->Listbox->GetWidgetName());

  // Update enable state

  this->UpdateEnableState();
}


//----------------------------------------------------------------------------
void vtkKWListBox::SetWidth(int w)
{
  this->Script("%s configure -width %d", this->Listbox->GetWidgetName(), w);
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetScrollbarFlag(int v)
{
  this->ScrollbarFlag = v;

  if (this->IsCreated())
    {
    this->Script("pack forget %s", this->Scrollbar->GetWidgetName());
    this->Script("pack forget %s", this->Listbox->GetWidgetName());
    if (this->ScrollbarFlag)
      {
      this->Script("pack %s -side right -fill y", this->Scrollbar->GetWidgetName());
      }
    this->Script("pack %s -side left -expand 1 -fill both", this->Listbox->GetWidgetName());
    }
}



//----------------------------------------------------------------------------
void vtkKWListBox::SetHeight(int h)
{
  this->Script("%s configure -height %d", this->Listbox->GetWidgetName(), h);
}

//----------------------------------------------------------------------------
void vtkKWListBox::DeleteAll()
{
  int n =  this->GetNumberOfItems();
  this->DeleteRange(0, n-1);
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetBalloonHelpString(const char *str)
{
  this->Listbox->SetBalloonHelpString( str );
  this->Scrollbar->SetBalloonHelpString( str );
}


//----------------------------------------------------------------------------
void vtkKWListBox::SetBalloonHelpJustification( int j )
{
  this->Listbox->SetBalloonHelpJustification( j );
  this->Scrollbar->SetBalloonHelpJustification( j );
}

//----------------------------------------------------------------------------
void vtkKWListBox::UpdateEnableState()
{
  //  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Scrollbar);
  this->PropagateEnableState(this->Listbox);
  if (this->Listbox)
    {
    this->Listbox->SetStateOption(this->Enabled);
    }
}
//----------------------------------------------------------------------------
void vtkKWListBox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "Listbox " << this->Listbox << endl;
}

