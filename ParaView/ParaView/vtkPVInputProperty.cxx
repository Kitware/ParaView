/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInputProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkCollection.h"
#include "vtkPVInputRequirement.h"
#include "vtkPVConfig.h"
#ifdef PARAVIEW_BUILD_DEVELOPMENT
#include "vtkCTHData.h"
#endif


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputProperty);
vtkCxxRevisionMacro(vtkPVInputProperty, "1.8.2.1");

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
  if (this->Type == NULL)
    {
    return 1;
    }
  if (strcmp(this->Type, "vtkDataSet") == 0)
    {
    if ( info->GetDataSetType() == VTK_DATA_SET ||
         info->GetDataSetType() == VTK_POINT_SET ||
         info->GetDataSetType() == VTK_IMAGE_DATA ||
         info->GetDataSetType() == VTK_RECTILINEAR_GRID ||
         info->GetDataSetType() == VTK_STRUCTURED_GRID ||
         info->GetDataSetType() == VTK_STRUCTURED_POINTS ||
         info->GetDataSetType() == VTK_POLY_DATA ||
         info->GetDataSetType() == VTK_UNSTRUCTURED_GRID ||
#ifdef PARAVIEW_BUILD_DEVELOPMENT
         info->GetDataSetType() == VTK_CTH_DATA ||
#endif
         info->GetDataSetType() == VTK_UNIFORM_GRID )
      {
      return 1;
      }
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
#ifdef PARAVIEW_BUILD_DEVELOPMENT
  if (strcmp(this->Type, "vtkCTHData") == 0 &&
      info->GetDataSetType() == VTK_CTH_DATA)
    {
    return 1;
    }
#endif
  if (strcmp(this->Type, "vtkHierarchicalBoxDataSet") == 0 &&
      info->GetDataSetType() == VTK_HIERARCHICAL_BOX_DATA_SET)
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


  



