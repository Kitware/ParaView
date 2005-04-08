/*=========================================================================

  Program:   ParaView
  Module:    vtkCVGeometryCache.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCVGeometryCache.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMapper.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkXMLPVDWriter.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkCVGeometryCache, "1.1.2.2");
vtkStandardNewMacro(vtkCVGeometryCache);

struct vtkCVGeometryCacheInternal
{
  typedef vtkstd::vector<vtkSmartPointer<vtkPolyData> > PolyDatasType;
  PolyDatasType PolyDatas;
};

//----------------------------------------------------------------------------
vtkCVGeometryCache::vtkCVGeometryCache()
{
  this->Internal = new vtkCVGeometryCacheInternal;
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkCVGeometryCache::~vtkCVGeometryCache()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
int vtkCVGeometryCache::RequestDataObject(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  unsigned int numOutputs = this->Internal->PolyDatas.size();
  this->SetNumberOfOutputPorts(numOutputs);
  for (unsigned int i=0; i<numOutputs; i++)
    {
    vtkInformation* outInfo = outputVector->GetInformationObject(i);
    vtkPolyData* pd = vtkPolyData::New();
    pd->SetPipelineInformation(outInfo);
    outInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(), pd->GetExtentType());
    pd->Delete();
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkCVGeometryCache::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  unsigned int numOutputs = this->GetNumberOfOutputPorts();
  for (unsigned int i=0; i<numOutputs; i++)
    {
    vtkInformation* outInfo = outputVector->GetInformationObject(i);
    vtkPolyData* output = vtkPolyData::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (output)
      {
      output->ShallowCopy(this->Internal->PolyDatas[i]);
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkCVGeometryCache::AddGeometry(vtkMapper* mapper)
{
  vtkPolyData* orig = vtkPolyData::SafeDownCast(mapper->GetInput());
  if (!orig)
    {
    return;
    }
  vtkPolyData* copy = vtkPolyData::New();
  copy->ShallowCopy(orig);
  this->Internal->PolyDatas.push_back(copy);
  copy->Delete();
}

//----------------------------------------------------------------------------
void vtkCVGeometryCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
