/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVolumePropertyWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVVolumePropertyWidget.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"

vtkStandardNewMacro(vtkPVVolumePropertyWidget);
vtkCxxRevisionMacro(vtkPVVolumePropertyWidget, "1.4");

vtkCxxSetObjectMacro(vtkPVVolumePropertyWidget, DataInformation,
                     vtkPVDataInformation);

vtkPVVolumePropertyWidget::vtkPVVolumePropertyWidget()
{
  this->DataInformation = NULL;
  this->ArrayName = 0;
  this->ScalarMode = vtkPVVolumePropertyWidget::POINT_FIELD_DATA;
}

// ---------------------------------------------------------------------------
vtkPVVolumePropertyWidget::~vtkPVVolumePropertyWidget()
{
  this->SetDataInformation( NULL );
  this->SetArrayName(0);
}



// ---------------------------------------------------------------------------
void vtkPVVolumePropertyWidget::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DataInformation: ";
  if( this->DataInformation )
    {
    this->DataInformation->PrintSelf( os << endl, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "ArrayName: " << ( (this->ArrayName)? this->ArrayName :
    "(null)" ) << endl;
  os << indent << "ScalarMode: " << this->ScalarMode << endl;
    
}


// ---------------------------------------------------------------------------
int vtkPVVolumePropertyWidget::GetDataSetNumberOfComponents()
{
  if (this->DataInformation && this->ArrayName)
    {
    vtkPVDataSetAttributesInformation *pdInfo = 
      (this->ScalarMode == vtkPVVolumePropertyWidget::POINT_FIELD_DATA)?
      this->DataInformation->GetPointDataInformation ():
        this->DataInformation->GetCellDataInformation();
    if( pdInfo )
      {
      vtkPVArrayInformation *arrayInfo = pdInfo->GetArrayInformation (this->ArrayName);
      return arrayInfo->GetNumberOfComponents();
      }
    }
  return 0;
}

// ---------------------------------------------------------------------------
int vtkPVVolumePropertyWidget::GetDataSetScalarRange(
  int comp, double range[2])
{
  if (this->DataInformation && this->ArrayName)
    {
    vtkPVDataSetAttributesInformation *pdInfo = 
      (this->ScalarMode == vtkPVVolumePropertyWidget::POINT_FIELD_DATA)?
      this->DataInformation->GetPointDataInformation ():
        this->DataInformation->GetCellDataInformation();
    
    if( pdInfo )
      {
      vtkPVArrayInformation *arrayInfo = pdInfo->GetArrayInformation (
        this->ArrayName);
      arrayInfo->GetComponentRange (comp, range);
      return 1;
      }
    }
  return 0;
}

// ---------------------------------------------------------------------------
int vtkPVVolumePropertyWidget::GetDataSetAdjustedScalarRange(
  int comp, double range[2])
{
  if (this->DataInformation && this->ArrayName)
    {
    vtkPVDataSetAttributesInformation *pdInfo = 
      (this->ScalarMode == vtkPVVolumePropertyWidget::POINT_FIELD_DATA)?
      this->DataInformation->GetPointDataInformation ():
        this->DataInformation->GetCellDataInformation();
    if( pdInfo )
      {
      vtkPVArrayInformation *arrayInfo = pdInfo->GetArrayInformation (this->ArrayName);
      arrayInfo->GetComponentRange (comp, range);
      switch( arrayInfo->GetDataType() )
        {
        case VTK_UNSIGNED_CHAR:
          arrayInfo->GetDataTypeRange(range);
          break;
        case VTK_UNSIGNED_SHORT:
          if( range[1] <= 4095.0 )
            {
            arrayInfo->GetDataTypeRange(range);
            range[1] = 4095.0;
            }
          else
            {
            arrayInfo->GetDataTypeRange(range);
            }
          break;
        }
      return 1;
      }
    }
  return 0;
}

// ---------------------------------------------------------------------------
const char* vtkPVVolumePropertyWidget::GetDataSetScalarName()
{
  return this->ArrayName;
}

// ---------------------------------------------------------------------------
int vtkPVVolumePropertyWidget::GetDataSetScalarOpacityUnitDistanceRangeAndResolution(
  double range[2], double *resolution)
{
 if (this->DataInformation)
    {
    double bounds[6];
    this->DataInformation->GetBounds(bounds);
    
    double diameter = 
      sqrt( (bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
            (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
            (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]) );
    
    int numCells = this->DataInformation->GetNumberOfCells();
    double linearNumCells = pow( (double) numCells, 1.0/3.0 );
    
    double soud_res = diameter / (linearNumCells * 10.0);
    *resolution = soud_res;

    range[0] = diameter / (linearNumCells * 10.0);
    range[1] = diameter / (linearNumCells / 10.0);

    return 1;
    }

  return 0;
}
