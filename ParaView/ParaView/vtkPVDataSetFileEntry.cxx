/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVDataSetFileEntry.cxx
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
#include "vtkPVApplication.h"
#include "vtkPVDataSetFileEntry.h"
#include "vtkPVWindow.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkPVDataSetFileEntry* vtkPVDataSetFileEntry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVDataSetFileEntry");
  if (ret)
    {
    return (vtkPVDataSetFileEntry*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVDataSetFileEntry;
}

//----------------------------------------------------------------------------
vtkPVDataSetFileEntry::vtkPVDataSetFileEntry()
{
  this->TypeReader = vtkDataSetReader::New();
  this->Type = -1;
}

//----------------------------------------------------------------------------
vtkPVDataSetFileEntry::~vtkPVDataSetFileEntry()
{
  this->TypeReader->Delete();
  this->TypeReader = NULL;
}



//----------------------------------------------------------------------------
void vtkPVDataSetFileEntry::Accept()
{
  int newType;

  this->TypeReader->SetFileName(this->GetValue());
  newType = this->TypeReader->ReadOutputType();

  if (newType == -1)
    {
    this->GetPVApplication()->GetMainWindow()->WarningMessage("Could not read file.");
    this->Reset();
    return;
    }
  if (this->Type == -1)
    {
    this->Type = newType;
    }
  if (this->Type != newType)
    {
    this->GetPVApplication()->GetMainWindow()->WarningMessage("Output type does not match previous file.");
    this->Reset();
    return;
    }

  this->vtkPVFileEntry::Accept();

}

vtkPVDataSetFileEntry* vtkPVDataSetFileEntry::ClonePrototype(
  vtkPVSource* pvSource, vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVDataSetFileEntry::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
int vtkPVDataSetFileEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                             vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  return 1;
}
