/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHDataArray.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCTHDataArray.h"
#include "vtkArrayIteratorTemplate.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"

#include <fstream>

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkCTHDataArray);

//---------------------------------------------------------------------------

vtkCTHDataArray::vtkCTHDataArray()
{
  this->ExtentsSet = false;
  this->PointerTime = 0;

  this->Data = nullptr;
  this->CopiedData = nullptr;
  this->Tuple = nullptr;
  this->TupleSize = 0;

  this->Fallback = vtkDoubleArray::New();
}

vtkCTHDataArray::~vtkCTHDataArray()
{
  this->Initialize();
}

void vtkCTHDataArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimensions " << this->Dimensions[0] << " " << this->Dimensions[1] << " "
     << this->Dimensions[2] << endl;
}

void vtkCTHDataArray::Initialize()
{
  if (this->Fallback)
  {
    this->Fallback->Delete();
    this->Fallback = nullptr;
  }

  if (this->Data)
  {
    for (int i = 0; i < this->GetNumberOfComponents(); i++)
    {
      if (this->Data[i])
      {
        delete[] this->Data[i];
      }
    }
    delete[] this->Data;
  }
  this->Data = nullptr;

  if (this->CopiedData)
  {
    delete[] this->CopiedData;
  }
  this->CopiedData = nullptr;

  if (this->Tuple)
  {
    delete[] this->Tuple;
  }
}

// This one sets the size for the data pointers
void vtkCTHDataArray::SetDimensions(int x, int y, int z)
{
  if (this->Fallback)
  {
    this->Fallback->Delete();
    this->Fallback = nullptr;
  }

  this->Dimensions[0] = x;
  this->Dimensions[1] = y;
  this->Dimensions[2] = z;
  this->MaxId = x * y * z - 1;
  int numComp = this->GetNumberOfComponents();
  this->Size = (MaxId + 1) * numComp;

  if (this->Data)
  {
    this->Initialize();
  }
  this->Data = new double**[numComp];
  for (int i = 0; i < numComp; i++)
  {
    this->Data[i] = new double*[y * z];
  }
  this->ExtentsSet = false;
}

// If this is called then it means we need to offset by some amount.
void vtkCTHDataArray::SetExtents(int x0, int x1, int y0, int y1, int z0, int z1)
{
  this->Dx = x1 - x0 + 1;
  this->Dy = y1 - y0 + 1;
  this->Dz = z1 - z0 + 1;
  this->Extents[0] = x0;
  this->Extents[1] = x1;
  this->Extents[2] = y0;
  this->Extents[3] = y1;
  this->Extents[4] = z0;
  this->Extents[5] = z1;
  this->MaxId = this->Dx * this->Dy * this->Dz - 1;

  int numComp = this->GetNumberOfComponents();
  this->Size = (MaxId + 1) * numComp;
  this->ExtentsSet = true;
}

void vtkCTHDataArray::UnsetExtents()
{
  this->MaxId = this->Dimensions[0] * this->Dimensions[1] * this->Dimensions[2] - 1;
  this->Size = (MaxId + 1) * this->GetNumberOfComponents();
  this->ExtentsSet = false;
}

void vtkCTHDataArray::SetDataPointer(int comp, int k, int j, double* istrip)
{
  this->Data[comp][k * this->Dimensions[1] + j] = istrip;
}

void vtkCTHDataArray::GetTuple(vtkIdType i, double* tuple)
{
  if (this->Fallback)
  {
    Fallback->GetTuple(i, tuple);
    return;
  }
  if (this->ExtentsSet)
  {
    int P = i / this->Dx;
    int Pk = P / this->Dy + this->Extents[4];
    int Pj = P % this->Dy + this->Extents[2];
    int plane = Pk * this->Dimensions[1] + Pj;
    int offset = i % this->Dx + this->Extents[0];
    for (int c = 0; c < this->GetNumberOfComponents(); c++)
    {
      tuple[c] = this->Data[c][plane][offset];
    }
  }
  else
  {
    int plane = i / this->Dimensions[0];
    int offset = i % this->Dimensions[0];
    for (int c = 0; c < this->GetNumberOfComponents(); c++)
    {
      tuple[c] = this->Data[c][plane][offset];
    }
  }
}

double* vtkCTHDataArray::GetTuple(vtkIdType i)
{
  if (this->Fallback)
  {
    return this->Fallback->GetTuple(i);
  }
  int numComp = this->GetNumberOfComponents();
  if (this->TupleSize != numComp)
  {
    if (this->Tuple)
    {
      delete[] this->Tuple;
      this->Tuple = nullptr;
    }
    // fall through to the next if(), i.e. the initial case
  }
  if (!this->Tuple)
  {
    this->TupleSize = numComp;
    this->Tuple = new double[this->TupleSize];
  }
  this->GetTuple(i, this->Tuple);
  return this->Tuple;
}

double* vtkCTHDataArray::GetPointer(vtkIdType id)
{
  if (this->Fallback)
  {
    return this->Fallback->GetPointer(id);
  }
  if (this->PointerTime < this->GetMTime())
  {
    int numComp = this->GetNumberOfComponents();
    int size;
    if (this->ExtentsSet)
    {
      size = numComp * Dx * Dy * Dz;
    }
    else
    {
      int plane = this->Dimensions[2] * this->Dimensions[1];
      size = numComp * plane * this->Dimensions[0];
    }
    if (this->CopiedData)
    {
      if (this->CopiedSize < size)
      {
        delete[] this->CopiedData;
        this->CopiedSize = size;
        this->CopiedData = new double[this->CopiedSize];
      }
    }
    else
    {
      this->CopiedSize = size;
      this->CopiedData = new double[this->CopiedSize];
    }
    this->ExportToVoidPointer(this->CopiedData);
    this->PointerTime = this->GetMTime();
  }
  return this->CopiedData + id;
}

void vtkCTHDataArray::ExportToVoidPointer(void* out_ptr)
{
  if (this->Fallback)
  {
    return this->Fallback->ExportToVoidPointer(out_ptr);
  }
  if (!out_ptr)
    return;
  double* out_data = static_cast<double*>(out_ptr);
  int rawIndex = 0;
  int numComp = this->GetNumberOfComponents();
  if (this->ExtentsSet)
  {
    for (int c = 0; c < numComp; c++)
    {
      for (int k = this->Extents[4]; k <= this->Extents[5]; k++)
      {
        for (int j = this->Extents[2]; j <= this->Extents[3]; j++)
        {
          int plane = k * this->Dimensions[1] + j;
          for (int i = this->Extents[0]; i <= this->Extents[1]; i++)
          {
            out_data[rawIndex++] = this->Data[c][plane][i];
          }
        }
      }
    }
  }
  else
  {
    int plane = this->Dimensions[2] * this->Dimensions[1];
    for (int c = 0; c < numComp; c++)
    {
      for (int kj = 0; kj < plane; kj++)
      {
        for (int i = 0; i < this->Dimensions[0]; i++)
        {
          out_data[rawIndex++] = this->Data[c][kj][i];
        }
      }
    }
  }
}

vtkIdType vtkCTHDataArray::LookupValue(vtkVariant var)
{
  if (this->Fallback)
  {
    return this->Fallback->LookupValue(var);
  }
  bool valid = true;
  double val = var.ToDouble(&valid);
  if (valid)
  {
    int numComp = this->GetNumberOfComponents();
    if (this->ExtentsSet)
    {
      for (int c = 0; c < numComp; c++)
      {
        for (int k = this->Extents[4]; k <= this->Extents[5]; k++)
        {
          for (int j = this->Extents[2]; k <= this->Extents[3]; k++)
          {
            int plane = k * this->Dy + j;
            for (int i = this->Extents[0]; i <= this->Extents[1]; i++)
            {
              if (this->Data[c][plane][i] == val)
              {
                return c * this->Dz * this->Dy * this->Dz + plane * this->Dx + i;
              }
            }
          }
        }
      }
    }
    else
    {
      int plane = this->Dimensions[2] * this->Dimensions[1];
      for (int c = 0; c < numComp; c++)
      {
        for (int kj = 0; kj < plane; kj++)
        {
          for (int i = 0; i < this->Dimensions[0]; i++)
          {
            if (this->Data[c][kj][i] == val)
            {
              return c * plane * this->Dimensions[0] + kj * this->Dimensions[0] + i;
            }
          }
        }
      }
    }
  }
  return -1;
}

void vtkCTHDataArray::LookupValue(vtkVariant var, vtkIdList* ids)
{
  if (this->Fallback)
  {
    this->Fallback->LookupValue(var, ids);
    return;
  }
  bool valid = true;
  double val = var.ToDouble(&valid);
  ids->Reset();
  if (valid)
  {
    int numComp = this->GetNumberOfComponents();
    if (this->ExtentsSet)
    {
      for (int c = 0; c < numComp; c++)
      {
        for (int k = this->Extents[4]; k <= this->Extents[5]; k++)
        {
          for (int j = this->Extents[2]; k <= this->Extents[3]; k++)
          {
            int plane = k * this->Dy + j;
            for (int i = this->Extents[0]; i <= this->Extents[1]; i++)
            {
              if (this->Data[c][plane][i] == val)
              {
                ids->InsertNextId(c * this->Dx * this->Dy * this->Dz + plane * this->Dx + i);
              }
            }
          }
        }
      }
    }
    else
    {
      int plane = this->Dimensions[2] * this->Dimensions[1];
      for (int c = 0; c < numComp; c++)
      {
        for (int kj = 0; kj < plane; kj++)
        {
          for (int i = 0; i < this->Dimensions[0]; i++)
          {
            if (this->Data[c][kj][i] == val)
            {
              ids->InsertNextId(c * plane * this->Dimensions[0] + kj * this->Dimensions[0] + i);
            }
          }
        }
      }
    }
  }
}

vtkArrayIterator* vtkCTHDataArray::NewIterator()
{
  if (!this->Fallback)
  {
    this->BuildFallback();
  }
  return this->Fallback->NewIterator();
}

void vtkCTHDataArray::BuildFallback()
{
  if (!Fallback)
  {
    vtkDoubleArray* da = vtkDoubleArray::New();
    da->SetNumberOfTuples(this->GetNumberOfTuples());
    da->SetNumberOfComponents(this->GetNumberOfComponents());
    // Avoid calling GetPointer so that we don't make an unnecessary copy.
    for (vtkIdType i = 0; i < this->Size; i++)
    {
      da->SetTypedTuple(i, this->GetTuple(i));
    }
    this->Fallback = da;
  }
}
