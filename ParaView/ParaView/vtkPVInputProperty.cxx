/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputProperty.cxx
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
#include "vtkPVInputProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkCollection.h"
#include "vtkPVInputRequirement.h"
#include "vtkCTHData.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputProperty);
vtkCxxRevisionMacro(vtkPVInputProperty, "1.4");

//----------------------------------------------------------------------------
vtkPVInputProperty::vtkPVInputProperty()
{
  this->Name = NULL;
  this->Type = NULL;
  this->Requirements = vtkCollection::New();
}

//----------------------------------------------------------------------------
vtkPVInputProperty::~vtkPVInputProperty()
{  
  this->SetName(NULL);
  this->SetType(NULL);
  this->Requirements->Delete();
  this->Requirements = NULL;
}

//----------------------------------------------------------------------------
void vtkPVInputProperty::AddRequirement(vtkPVInputRequirement *ir)
{
  this->Requirements->AddItem(ir);
}

//----------------------------------------------------------------------------
int vtkPVInputProperty::GetIsValidInput(vtkPVSource *input, vtkPVSource *pvs)
{
  vtkPVDataInformation *info;
  vtkPVInputRequirement *ir;

  if (input->GetPVOutput() == NULL)
    {
    return 0;
    }

  info = input->GetDataInformation();

  // First check the data has the correct arrays.
  this->Requirements->InitTraversal();
  while ( (ir = (vtkPVInputRequirement*)(this->Requirements->GetNextItemAsObject())))
    {
    if ( ! ir->GetIsValidInput(input, pvs) )
      {
      return 0;
      }
    }

  // vtkPVDataInformation has a similar check.
  // We should merge DataInformations check with this !!!!!! ....

  // Also, this could be just another requirment instead of an input property attribute.
 
  // Special sets of types.
  if (this->Type == NULL || strcmp(this->Type, "vtkDataSet") == 0)
    {
    return 1;
    }
  if (strcmp(this->Type, "vtkStructuredData") == 0)
    {
    if (info->GetDataSetType() == VTK_IMAGE_DATA ||
        info->GetDataSetType() == VTK_RECTILINEAR_GRID ||
        info->GetDataSetType() == VTK_STRUCTURED_GRID)
      {
      return 1;
      }
    }
  if (strcmp(this->Type, "vtkPointSet") == 0)
    {
    if (info->GetDataSetType() == VTK_POLY_DATA ||
        info->GetDataSetType() == VTK_UNSTRUCTURED_GRID ||
        info->GetDataSetType() == VTK_STRUCTURED_GRID)
      {
      return 1;
      }
    }

  // Standard matches
  if (strcmp(this->Type, "vtkPolyData") == 0 &&
      info->GetDataSetType() == VTK_POLY_DATA)
    {
    return 1;
    }
  if (strcmp(this->Type, "vtkUnstructuredGrid") == 0 &&
      info->GetDataSetType() == VTK_UNSTRUCTURED_GRID)
    {
    return 1;
    }
  if (strcmp(this->Type, "vtkImageData") == 0 &&
      info->GetDataSetType() == VTK_IMAGE_DATA)
    {
    return 1;
    }
  if (strcmp(this->Type, "vtkRectilinearGrid") == 0 &&
      info->GetDataSetType() == VTK_RECTILINEAR_GRID)
    {
    return 1;
    }
  if (strcmp(this->Type, "vtkStructuredGrid") == 0 &&
      info->GetDataSetType() == VTK_STRUCTURED_GRID)
    {
    return 1;
    }
  if (strcmp(this->Type, "vtkCTHData") == 0 &&
      info->GetDataSetType() == VTK_CTH_DATA)
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVInputProperty::GetIsValidField(int field, 
                                 vtkPVDataSetAttributesInformation* fieldInfo)
{
  vtkPVInputRequirement *ir;

  // First check the field has the correct arrays.
  this->Requirements->InitTraversal();
  while ( (ir = (vtkPVInputRequirement*)(this->Requirements->GetNextItemAsObject())))
    {
    if ( ! ir->GetIsValidField(field, fieldInfo) )
      {
      return 0;
      }
    }

  return 1;
}



//----------------------------------------------------------------------------
void vtkPVInputProperty::Copy(vtkPVInputProperty *in)
{
  this->SetName(in->GetName());
  this->SetType(in->GetType());
  this->Requirements->RemoveAllItems();
  
  vtkPVInputRequirement *ir;
  in->Requirements->InitTraversal();
  while ( (ir = (vtkPVInputRequirement*)(in->Requirements->GetNextItemAsObject())))
    {
    this->AddRequirement(ir);
    }
}

//----------------------------------------------------------------------------
void vtkPVInputProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if (this->Name)
    {
    os << indent << "Name: " << this->Name << endl;
    }
  if (this->Type)
    {
    os << indent << "Type: " << this->Type << endl;
    }
}


  



