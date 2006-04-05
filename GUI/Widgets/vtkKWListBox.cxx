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

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWListBox);
vtkCxxRevisionMacro(vtkKWListBox, "1.49");

//----------------------------------------------------------------------------
vtkKWListBox::vtkKWListBox()
{   
  this->CurrentSelection = 0;
  this->Item = 0; 
}

//----------------------------------------------------------------------------
vtkKWListBox::~vtkKWListBox()
{
  delete [] this->Item;
  delete [] this->CurrentSelection;
}

//----------------------------------------------------------------------------
void vtkKWListBox::Create()
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget("listbox"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
int vtkKWListBox::GetNumberOfItems()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  return atoi(this->Script("%s size", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWListBox::DeleteRange(int start, int end)
{
  int enabled = this->GetEnabled();
  this->SetEnabled(1);
  this->Script("%s delete %d %d", this->GetWidgetName(), start, end);
  this->SetEnabled(enabled);
}

//----------------------------------------------------------------------------
const char* vtkKWListBox::GetItem(int index)
{
  const char* result = 
    this->Script("%s get %d", this->GetWidgetName(), index);
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
  this->Script("%s selection set %d", this->GetWidgetName(), sel);
}

//----------------------------------------------------------------------------
int vtkKWListBox::GetSelectionIndex()
{
  if ( !this->IsCreated() )
    {
    return 0;
    }
  const char* result = this->Script(
    "%s curselection", this->GetWidgetName(), this->GetWidgetName());
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
    this->GetWidgetName(),
    this->GetWidgetName());

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

  int was_disabled = !this->GetEnabled();
  if (was_disabled)
    {
    this->SetEnabled(1);
    }
  if (state)
    {
    this->Script("%s selection set %d", this->GetWidgetName(), idx);
    }
  else
    {
    this->Script("%s selection clear %d", this->GetWidgetName(), idx);
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
                           this->GetWidgetName(), idx));

}

//----------------------------------------------------------------------------
void vtkKWListBox::InsertEntry(int index, const char *name)
{
  if ( !this->IsCreated() )
    {
    return;
    }
  int enabled = this->GetEnabled();
  if ( !enabled )
    {
    this->SetEnabled(1);
    }
  this->Script("%s insert %d {%s}", this->GetWidgetName(), index, name);
  if ( !enabled )
    {
    this->SetEnabled(0);
    }
}


 
//----------------------------------------------------------------------------
void vtkKWListBox::SetDoubleClickCommand(vtkObject *obj, 
                                          const char *method)
{
  this->SetBinding("<Double-1>", obj, method);
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetSingleClickCommand(vtkObject *obj, 
                                          const char *method)
{
  this->SetBinding("<ButtonRelease-1>", obj, method);
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
    return this->Append(name);
    }
  return !found;
}



//----------------------------------------------------------------------------
int vtkKWListBox::Append(const char* name)
{
  int size = this->GetNumberOfItems();
  this->InsertEntry(size, name);
  return 1;
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetWidth(int w)
{
  this->SetConfigurationOptionAsInt("-width", w);
}

//----------------------------------------------------------------------------
int vtkKWListBox::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetHeight(int h)
{
  this->SetConfigurationOptionAsInt("-height", h);
}

//----------------------------------------------------------------------------
int vtkKWListBox::GetHeight()
{
  return this->GetConfigurationOptionAsInt("-height");
}

//----------------------------------------------------------------------------
void vtkKWListBox::DeleteAll()
{
  int n =  this->GetNumberOfItems();
  this->DeleteRange(0, n-1);
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetSelectionMode(int relief)
{
  this->SetConfigurationOption(
    "-selectmode", vtkKWTkOptions::GetSelectionModeAsTkOptionValue(relief));
}

void vtkKWListBox::SetSelectionModeToSingle() 
{ 
  this->SetSelectionMode(vtkKWTkOptions::SelectionModeSingle); 
};
void vtkKWListBox::SetSelectionModeToBrowse() 
{ 
  this->SetSelectionMode(vtkKWTkOptions::SelectionModeBrowse); 
};
void vtkKWListBox::SetSelectionModeToMultiple() 
{ 
  this->SetSelectionMode(vtkKWTkOptions::SelectionModeMultiple); 
};
void vtkKWListBox::SetSelectionModeToExtended() 
{ 
  this->SetSelectionMode(vtkKWTkOptions::SelectionModeExtended); 
};

//----------------------------------------------------------------------------
int vtkKWListBox::GetSelectionMode()
{
  return vtkKWTkOptions::GetSelectionModeFromTkOptionValue(
    this->GetConfigurationOption("-selectmode"));
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetExportSelection(int arg)
{
  this->SetConfigurationOptionAsInt("-exportselection", arg);
}

//----------------------------------------------------------------------------
int vtkKWListBox::GetExportSelection()
{
  return this->GetConfigurationOptionAsInt("-exportselection");
}

//----------------------------------------------------------------------------
void vtkKWListBox::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWListBox::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWListBox::GetForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWListBox::GetForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-foreground");
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWListBox::GetDisabledForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWListBox::GetDisabledForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-disabledforeground");
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetDisabledForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWListBox::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWListBox::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWListBox::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWTkOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWListBox::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRaised); 
};
void vtkKWListBox::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSunken); 
};
void vtkKWListBox::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefFlat); 
};
void vtkKWListBox::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRidge); 
};
void vtkKWListBox::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSolid); 
};
void vtkKWListBox::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWListBox::GetRelief()
{
  return vtkKWTkOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWListBox::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetState(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWListBox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

