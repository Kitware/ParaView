/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputArrayRequirement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInputArrayRequirement.h"

#include "vtkObjectFactory.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkDataSet.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLPackageParser.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputArrayRequirement);
vtkCxxRevisionMacro(vtkPVInputArrayRequirement, "1.6");

//----------------------------------------------------------------------------
vtkPVInputArrayRequirement::vtkPVInputArrayRequirement()
{
  this->Attribute = -1;
  this->DataType = -1;
  this->NumberOfComponents = -1;
}

//----------------------------------------------------------------------------
int vtkPVInputArrayRequirement::ReadXMLAttributes(vtkPVXMLElement* element,
                                                 vtkPVXMLPackageParser*)
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
int vtkPVInputArrayRequirement::GetIsValidInput(vtkPVSource* input, vtkPVSource*)
{
  vtkPVDataInformation *info = input->GetDataInformation();
  
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


  



