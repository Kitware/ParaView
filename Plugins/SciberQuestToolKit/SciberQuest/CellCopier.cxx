/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "CellCopier.h"

#include "DataArrayCopierSpecializations.hxx"

#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"

typedef std::pair<std::map<vtkIdType,vtkIdType>::iterator,bool> MapInsert;
typedef std::pair<vtkIdType,vtkIdType> MapElement;

//*****************************************************************************
DataArrayCopier *NewDataArrayCopier(vtkDataArray *da)
{
  DataArrayCopier *dac;
  switch (da->GetDataType())
    {
    case VTK_INT:
      dac=new IntDataArrayCopier;
      break;

    case VTK_ID_TYPE:
      dac=new IdTypeDataArrayCopier;
      break;

    case VTK_FLOAT:
      dac=new FloatDataArrayCopier;
      break;

    case VTK_DOUBLE:
      dac=new DoubleDataArrayCopier;
      break;

    case VTK_UNSIGNED_CHAR:
      dac=new UnsignedCharDataArrayCopier;
      break;

    default:
      sqErrorMacro(std::cerr,"Unsupported array type.");
      return 0;
    }
  return dac;
}

//-----------------------------------------------------------------------------
CellCopier::~CellCopier()
{
  this->ClearDataCopier();
}

//-----------------------------------------------------------------------------
void CellCopier::ClearDataCopier()
{
  size_t n;
  n=this->PointDataCopier.size();
  for (size_t i=0; i<n; ++i)
    {
    delete this->PointDataCopier[i];
    }
  this->PointDataCopier.clear();

  n=this->CellDataCopier.size();
  for (size_t i=0; i<n; ++i)
    {
    delete this->CellDataCopier[i];
    }
  this->CellDataCopier.clear();
}

//-----------------------------------------------------------------------------
void CellCopier::Initialize(vtkDataSet *in, vtkDataSet *out)
{
  this->ClearDataCopier();
  this->ClearPointIdMap();

  size_t n;
  vtkPointData *pdIn=in->GetPointData();
  vtkPointData *pdOut=out->GetPointData();
  n=pdIn->GetNumberOfArrays();
  for (size_t i=0; i<n; ++i)
    {
    vtkDataArray *da=pdIn->GetArray((int)i);
    DataArrayCopier *dac=NewDataArrayCopier(da);
    dac->Initialize(da);
    this->PointDataCopier.push_back(dac);
    pdOut->AddArray(dac->GetOutput());
    }

  vtkCellData *cdIn=in->GetCellData();
  vtkCellData *cdOut=out->GetCellData();
  n=cdIn->GetNumberOfArrays();
  for (size_t i=0; i<n; ++i)
    {
    vtkDataArray *da=cdIn->GetArray((int)i);
    DataArrayCopier *dac=NewDataArrayCopier(da);
    dac->Initialize(da);
    this->CellDataCopier.push_back(dac);
    cdOut->AddArray(dac->GetOutput());
    }
}

//-----------------------------------------------------------------------------
int CellCopier::CopyPointData(IdBlock &block)
{
  size_t n=this->PointDataCopier.size();
  for (size_t i=0; i<n; ++i)
    {
    this->PointDataCopier[i]->Copy(block);
    }
  return 1;
}

//-----------------------------------------------------------------------------
int CellCopier::CopyCellData(IdBlock &block)
{
  size_t n=this->CellDataCopier.size();
  for (size_t i=0; i<n; ++i)
    {
    this->CellDataCopier[i]->Copy(block);
    }
  return 1;
}

//-----------------------------------------------------------------------------
int CellCopier::CopyPointData(vtkIdType id)
{
  size_t n=this->PointDataCopier.size();
  for (size_t i=0; i<n; ++i)
    {
    this->PointDataCopier[i]->Copy(id);
    }
  return 1;
}

//-----------------------------------------------------------------------------
int CellCopier::CopyCellData(vtkIdType id)
{
  size_t n=this->CellDataCopier.size();
  for (size_t i=0; i<n; ++i)
    {
    this->CellDataCopier[i]->Copy(id);
    }
  return 1;
}

//-----------------------------------------------------------------------------
bool CellCopier::GetUniquePointId(vtkIdType inputId, vtkIdType &outputId)
{
  MapElement elem(inputId,outputId);
  MapInsert ret=this->PointIdMap.insert(elem);

  // existing or new point id
  outputId=(*ret.first).second;

  // true if insert succeded
  return ret.second;
}
