/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFileEntryProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVFileEntryProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVFileEntry.h"

#include <vtkstd/string>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVFileEntryProperty);
vtkCxxRevisionMacro(vtkPVFileEntryProperty, "1.5");

class vtkPVFileEntryPropertyList : vtkstd::vector<vtkstd::string>
{
  typedef vtkstd::vector<vtkstd::string> ListType;
public:
  void AddFile(const char* file)
    {
    ListType::iterator pos = this->begin();
    while ( pos != this->end() )
      {
      if ( *pos == file )
        {
        break;
        }
      ++pos;
      }
    if ( pos == this->end() )
      {
      this->push_back(file);
      }
    }

  int GetNumberOfItems()
    {
    return this->size();
    }

  const char* GetFile(int idx)
    {
    if ( idx < 0 || idx >= this->GetNumberOfItems() )
      {
      return 0;
      }
    return (this->begin() + idx)->c_str();
    }

  void RemoveFile(const char* file)
    {
    ListType::iterator pos = this->begin();
    while ( pos != this->end() )
      {
      if ( *pos == file )
        {
        break;
        }
      ++pos;
      }
    if ( pos != this->end() )
      {
      this->erase(pos);
      }
    }

  void RemoveAllFiles()
    {
    this->erase(this->begin(), this->end());
    }
};

//----------------------------------------------------------------------------
vtkPVFileEntryProperty::vtkPVFileEntryProperty()
{
  this->TimeStep = 0;
  this->DirectoryName = 0;
  this->Files = new vtkPVFileEntryPropertyList;
}

//----------------------------------------------------------------------------
vtkPVFileEntryProperty::~vtkPVFileEntryProperty()
{
  delete this->Files;
  this->Files = 0;
  this->SetDirectoryName(0);
}

//----------------------------------------------------------------------------
void vtkPVFileEntryProperty::SetAnimationTime(float time)
{
  vtkPVFileEntry *widget = vtkPVFileEntry::SafeDownCast(this->Widget);
  if (!widget)
    {
    return;
    }
  
  this->SetTimeStep(static_cast<int>(time));
  widget->Reset();
}

//----------------------------------------------------------------------------
const char* vtkPVFileEntryProperty::GetFile(int idx)
{
  return this->Files->GetFile(idx);
}

//----------------------------------------------------------------------------
void vtkPVFileEntryProperty::AddFile(const char* file)
{
  this->Files->AddFile(file);
}

//----------------------------------------------------------------------------
void vtkPVFileEntryProperty::RemoveAllFiles()
{
  this->Files->RemoveAllFiles();
}

//----------------------------------------------------------------------------
int vtkPVFileEntryProperty::GetNumberOfFiles()
{
  return this->Files->GetNumberOfItems();
}

//----------------------------------------------------------------------------
void vtkPVFileEntryProperty::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TimeStep: " << this->TimeStep << endl;
}
