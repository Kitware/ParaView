/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLookupTable.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLookupTable.h"

#include "vtkObjectFactory.h"
#include "vtkLookupTable.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVLookupTable);
vtkCxxRevisionMacro(vtkPVLookupTable, "1.4");
//-----------------------------------------------------------------------------
vtkPVLookupTable::vtkPVLookupTable()
{
  this->LookupTable = vtkLookupTable::New();

  this->Discretize = 0;
  this->NumberOfValues = 256;
  this->ScalePointsWithRange = 1;
}

//-----------------------------------------------------------------------------
vtkPVLookupTable::~vtkPVLookupTable()
{
  this->LookupTable->Delete();
}

struct vtkPVLookupTableNode
{
  double Value[6];
};

//-----------------------------------------------------------------------------
void vtkPVLookupTable::SetRange(double min, double max)
{
  if (!this->ScalePointsWithRange)
    {
    // If ScalePointsWithRange if off, all SetRange requests are simply ignored.
    // The range is determined by the control points added to the 
    // ColorTransferFunction.
    return;
    }

  // Scale the ColorTransferFunction control points over the new range.
  double oldrange[2];
  this->GetRange(oldrange);

  if (oldrange[0] == min && oldrange[1] == max)
    {
    // no range change, nothing to do.
    return;
    }


  // Adjust vtkColorTransferFunction points to the new range.
  double dold = (oldrange[1] - oldrange[0]);
  dold = (dold > 0) ? dold : 1;

  double dnew = (max -min);
  dnew = (dnew > 0) ? dnew : 1;

  double scale = dnew/dold;

  // Get the current control points.
  vtkstd::vector<vtkPVLookupTableNode> controlPoints;
  int num_points = this->GetSize();
  for (int cc=0; cc < num_points; cc++)
    {
    vtkPVLookupTableNode node;
    this->GetNodeValue(cc, node.Value);
    node.Value[0] = scale*(node.Value[0] - oldrange[0]) + min;
    controlPoints.push_back(node);
    }

  // Remove old points and add the moved points.
  this->RemoveAllPoints();

  // Now added the control points again.
  vtkstd::vector<vtkPVLookupTableNode>::iterator iter;
  for (iter = controlPoints.begin(); iter != controlPoints.end(); ++iter)
    {
    this->AddRGBPoint(iter->Value[0], iter->Value[1], iter->Value[2], 
      iter->Value[3], iter->Value[4], iter->Value[5]);
    }
  
  this->Build();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::SetNumberOfValues(vtkIdType number)
{
  this->NumberOfValues = number;
  this->LookupTable->SetNumberOfTableValues(number);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::Build()
{
  this->Superclass::Build();

  if (this->Discretize && (this->GetMTime() > this->BuildTime ||
    this->GetMTime() > this->BuildTime))
    {
    unsigned char* lut_ptr = this->LookupTable->WritePointer(0,
      this->NumberOfValues);
    double* table = new double[this->NumberOfValues*3];
    double range[2];
    this->LookupTable->GetTableRange(range);
    this->GetTable(range[0], range[1], this->NumberOfValues, table);
    // Now, convert double to unsigned chars and fill the LUT.
    for (int cc=0; cc < this->NumberOfValues; cc++)
      {
      lut_ptr[4*cc]   = (unsigned char)(255.0*table[3*cc] + 0.5);
      lut_ptr[4*cc+1] = (unsigned char)(255.0*table[3*cc+1] + 0.5);
      lut_ptr[4*cc+2] = (unsigned char)(255.0*table[3*cc+2] + 0.5);
      lut_ptr[4*cc+3] = 255;
      }
    delete [] table;

    this->LookupTable->SetRange(this->GetRange());
    this->BuildTime.Modified();
    }
}

//-----------------------------------------------------------------------------
unsigned char* vtkPVLookupTable::MapValue(double v)
{
  this->Build();
  if (this->Discretize)
    {
    return this->LookupTable->MapValue(v);
    }

  return this->Superclass::MapValue(v);
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::GetColor(double v, double rgb[3])
{
  this->Build();
  if (this->Discretize)
    {
    this->LookupTable->GetColor(v, rgb);
    return;
    }

  this->Superclass::GetColor(v, rgb);
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::MapScalarsThroughTable2(void *input, 
  unsigned char *output, int inputDataType, int numberOfValues,
  int inputIncrement, int outputFormat)
{
  if (this->Discretize)
    {
    // Make sure the LUT is built.
    this->Build();
    this->LookupTable->MapScalarsThroughTable2(
      input, output, inputDataType, numberOfValues, inputIncrement, 
      outputFormat);
    }
  else
    {
    this->Superclass::MapScalarsThroughTable2(
      input, output, inputDataType, numberOfValues, inputIncrement,
      outputFormat);
    }
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ScalePointsWithRange: " 
    << this->ScalePointsWithRange << endl;
  os << indent << "Discretize: " << this->Discretize << endl;
  os << indent << "NumberOfValues: " << this->NumberOfValues << endl;
}
