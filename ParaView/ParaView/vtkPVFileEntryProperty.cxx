/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVFileEntryProperty.cxx
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
#include "vtkPVFileEntryProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVFileEntry.h"

#include <vtkstd/string>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVFileEntryProperty);
vtkCxxRevisionMacro(vtkPVFileEntryProperty, "1.3");

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
  this->Files = new vtkPVFileEntryPropertyList;
}

//----------------------------------------------------------------------------
vtkPVFileEntryProperty::~vtkPVFileEntryProperty()
{
  delete this->Files;
  this->Files = 0;
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
