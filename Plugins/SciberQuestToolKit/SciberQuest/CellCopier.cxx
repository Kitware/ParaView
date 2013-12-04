/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

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
