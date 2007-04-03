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
vtkCxxRevisionMacro(vtkPVLookupTable, "1.7");
//-----------------------------------------------------------------------------
vtkPVLookupTable::vtkPVLookupTable()
{
  this->LookupTable = vtkLookupTable::New();

  this->Discretize = 0;
  this->NumberOfValues = 256;

  this->Data = 0;
  this->UseLogScale = 0;
}

//-----------------------------------------------------------------------------
vtkPVLookupTable::~vtkPVLookupTable()
{
  this->LookupTable->Delete();
  delete [] this->Data;
}

//-----------------------------------------------------------------------------
struct vtkPVLookupTableNode
{
  double Value[6];
};

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
    this->GetRange(range);
    this->LookupTable->SetRange(range);

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

    this->BuildTime.Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::SetAlpha(double alpha)
{
  this->LookupTable->SetAlpha(alpha);
  this->Superclass::SetAlpha(alpha);
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
double* vtkPVLookupTable::GetRGBPoints()
{
  delete [] this->Data;
  this->Data = 0;

  int num_points = this->GetSize();
  if (num_points > 0)
    {
    this->Data = new double[num_points*4];
    for (int cc=0; cc < num_points; cc++)
      {
      double values[6];
      this->GetNodeValue(cc, values);
      this->Data[4*cc] = values[0];
      this->Data[4*cc+1] = values[0];
      this->Data[4*cc+2] = values[1];
      this->Data[4*cc+3] = values[2];
      }
    }
  return this->Data;
}

//-----------------------------------------------------------------------------
void vtkPVLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Discretize: " << this->Discretize << endl;
  os << indent << "NumberOfValues: " << this->NumberOfValues << endl;
  os << indent << "UseLogScale: " << this->UseLogScale << endl;
}
