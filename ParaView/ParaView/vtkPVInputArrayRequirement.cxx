/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputArrayRequirement.cxx
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
#include "vtkPVInputArrayRequirement.h"

#include "vtkObjectFactory.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkDataSet.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLPackageParser.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputArrayRequirement);
vtkCxxRevisionMacro(vtkPVInputArrayRequirement, "1.3");

//----------------------------------------------------------------------------
vtkPVInputArrayRequirement::vtkPVInputArrayRequirement()
{
  this->Attribute = -1;
  this->DataType = -1;
  this->NumberOfComponents = -1;
}

//----------------------------------------------------------------------------
int vtkPVInputArrayRequirement::ReadXMLAttributes(vtkPVXMLElement* element,
                                                 vtkPVXMLPackageParser* parser)
{
  const char* rAttr;

  rAttr = element->GetAttribute("attribute");
  if (rAttr) 
    {
    if (strcmp(rAttr, "Point") == 0)
      {
      this->Attribute = vtkDataSet::POINT_DATA_FIELD;;
      }
    else if (strcmp(rAttr, "Cell") == 0)
      {
      this->Attribute = vtkDataSet::CELL_DATA_FIELD;;
      }
    else 
      {
      vtkErrorMacro("Unknown input attribute type: " << rAttr);
      }
    } 
  
  rAttr = element->GetAttribute("data_type");
  if (rAttr) 
    {
    if (strcmp(rAttr, "Float") == 0)
      {
      this->DataType = VTK_FLOAT;
      }
    if (strcmp(rAttr, "Double") == 0)
      {
      this->DataType = VTK_DOUBLE;
      }
    if (strcmp(rAttr, "Int") == 0)
      {
      this->DataType = VTK_INT;
      }
    if (strcmp(rAttr, "Long") == 0)
      {
      this->DataType = VTK_LONG;
      }
    if (strcmp(rAttr, "Char") == 0)
      {
      this->DataType = VTK_CHAR;
      }
    if (strcmp(rAttr, "UnsignedInt") == 0)
      {
      this->DataType = VTK_UNSIGNED_INT;
      }
    if (strcmp(rAttr, "UnsignedLong") == 0)
      {
      this->DataType = VTK_UNSIGNED_LONG;
      }
    if (strcmp(rAttr, "UnsignedChar") == 0)
      {
      this->DataType = VTK_UNSIGNED_CHAR;
      }
    }

  rAttr = element->GetAttribute("components");
  if (rAttr) 
    {
    this->NumberOfComponents = atoi(rAttr);
    }

  return 1;
}



//----------------------------------------------------------------------------
int vtkPVInputArrayRequirement::GetIsValidInput(vtkPVData* pvd, vtkPVSource*)
{
  vtkPVDataInformation *info = pvd->GetDataInformation();
  
  if (this->Attribute == vtkDataSet::POINT_DATA_FIELD)
    {
    return this->AttributeInfoContainsArray(info->GetPointDataInformation());
    }  
  if (this->Attribute == vtkDataSet::CELL_DATA_FIELD)
    {
    return this->AttributeInfoContainsArray(info->GetCellDataInformation());
    }  
  if (this->Attribute == vtkDataSet::DATA_OBJECT_FIELD)
    {
    vtkErrorMacro("Field restriction not implemented yet.");
    return 1;
    }  

  // No attribute limitation.
  if (this->AttributeInfoContainsArray(info->GetPointDataInformation()) )
    {
    return 1;
    }
  if (this->AttributeInfoContainsArray(info->GetCellDataInformation()) )
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVInputArrayRequirement::GetIsValidField(int field, 
                                 vtkPVDataSetAttributesInformation* fieldInfo)
{  
  // If attribute does not match ???
  if (this->Attribute != -1 && this->Attribute != field)
    {
    return 1;
    } 

  if (field == vtkDataSet::POINT_DATA_FIELD)
    {
    return this->AttributeInfoContainsArray(fieldInfo);
    }  
  if (field == vtkDataSet::CELL_DATA_FIELD)
    {
    return this->AttributeInfoContainsArray(fieldInfo);
    }  
  if (field == vtkDataSet::DATA_OBJECT_FIELD)
    {
    vtkErrorMacro("Field restriction not implemented yet.");
    }  
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVInputArrayRequirement::AttributeInfoContainsArray(
                                  vtkPVDataSetAttributesInformation* attrInfo)
{
  int pass;
  int num, idx;
  vtkPVArrayInformation *arrayInfo;

  num = attrInfo->GetNumberOfArrays();
  for (idx = 0; idx < num; ++idx)
    {
    pass = 1;
    arrayInfo = attrInfo->GetArrayInformation(idx);
    if (this->DataType >= 0 && this->DataType != arrayInfo->GetDataType())
      {
      pass = 0;
      }
    if (this->NumberOfComponents >= 0 && this->NumberOfComponents != arrayInfo->GetNumberOfComponents())
      {
      pass = 0;
      }
    if (pass)
      {
      return 1;
      }
    }

  return 0;
}


//----------------------------------------------------------------------------
void vtkPVInputArrayRequirement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if (this->Attribute == vtkDataSet::DATA_OBJECT_FIELD)
    {
    os << indent << "Attribute: DataObjectField \n";
    }
  if (this->Attribute == vtkDataSet::POINT_DATA_FIELD)
    {
    os << indent << "Attribute: PointData \n";
    }
  if (this->Attribute == vtkDataSet::CELL_DATA_FIELD)
    {
    os << indent << "Attribute: CellData \n";
    }
  if (this->DataType >= 0)
    {
    os << indent << "DataType: " << this->DataType << endl;
    }
  if (this->NumberOfComponents >= 0)
    {
    os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
    }
}


  



