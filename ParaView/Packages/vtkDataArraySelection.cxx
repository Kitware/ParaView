/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArraySelection.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataArraySelection.h"
#include "vtkObjectFactory.h"
#include "vtkVector.txx"

vtkCxxRevisionMacro(vtkDataArraySelection, "1.1");
vtkStandardNewMacro(vtkDataArraySelection);

//----------------------------------------------------------------------------
vtkDataArraySelection::vtkDataArraySelection()
{
  this->ArrayNames = ArrayNamesType::New();
  this->ArraySettings = ArraySettingsType::New();
}

//----------------------------------------------------------------------------
vtkDataArraySelection::~vtkDataArraySelection()
{
  this->ArraySettings->Delete();
  this->ArrayNames->Delete();
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number of Arrays: " << this->GetNumberOfArrays() << "\n";
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::EnableArray(const char* name)
{
  int pos=0;
  if(this->ArrayNames->FindItem(name, pos) == VTK_OK)
    {
    this->ArraySettings->SetItem(pos, 1);
    }
  this->ArrayNames->AppendItem(name);
  this->ArraySettings->AppendItem(1);
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::DisableArray(const char* name)
{
  int pos=0;
  if(this->ArrayNames->FindItem(name, pos) == VTK_OK)
    {
    this->ArraySettings->SetItem(pos, 0);
    }
  this->ArrayNames->AppendItem(name);
  this->ArraySettings->AppendItem(0);
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::ArrayIsEnabled(const char* name)
{
  // Check if there is a specific entry for this array.
  int pos=0;
  int result=0;
  if((this->ArrayNames->FindItem(name, pos) == VTK_OK) &&
     (this->ArraySettings->GetItem(pos, result) == VTK_OK))
    {
    return result;
    }
  
  // The array does not have an entry.  Assume it is disabled.
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::EnableAllArrays()
{
  vtkIdType i;
  for(i=0;i < this->ArraySettings->GetNumberOfItems();++i)
    {
    this->ArraySettings->SetItem(i, 1);
    }
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::DisableAllArrays()
{
  vtkIdType i;
  for(i=0;i < this->ArraySettings->GetNumberOfItems();++i)
    {
    this->ArraySettings->SetItem(i, 0);
    }
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::GetNumberOfArrays()
{
  return this->ArrayNames->GetNumberOfItems();
}

//----------------------------------------------------------------------------
const char* vtkDataArraySelection::GetArrayName(int index)
{
  const char* n = 0;
  if(this->ArrayNames->GetItem(index, n) == VTK_OK)
    {
    return n;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::GetArraySetting(int index)
{
  int n = 0;
  if(this->ArraySettings->GetItem(index, n) == VTK_OK)
    {
    return n;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::RemoveAllArrays()
{
  this->ArrayNames->RemoveAllItems();
  this->ArraySettings->RemoveAllItems();
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::SetArrays(const char** names, int numArrays)
{
  // Create a new map for this set of arrays.
  ArrayNamesType* newNames = ArrayNamesType::New();
  ArraySettingsType* newSettings = ArraySettingsType::New();
  
  // Allocate.
  newNames->SetSize(numArrays);
  newSettings->SetSize(numArrays);
  newNames->ResizeOn();
  newSettings->ResizeOn();
  
  // Fill with settings for all arrays.
  int i;
  for(i=0;i < numArrays; ++i)
    {
    // Add this array.
    newNames->AppendItem(names[i]);
    
    // Fill in the setting.
    int pos=0;
    int result=0;
    if((this->ArrayNames->FindItem(names[i], pos) == VTK_OK) &&
       (this->ArraySettings->GetItem(pos, result) == VTK_OK))
      {
      // Copy the old setting for this array.
      newSettings->AppendItem(result);
      }
    else
      {
      // No setting existed, default to on.
      newSettings->AppendItem(1);
      }
    }
  
  // Delete the old map and save the new one.
  this->ArrayNames->Delete();
  this->ArraySettings->Delete();
  this->ArrayNames = newNames;
  this->ArraySettings = newSettings;
}
