/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectDataSets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectDataSets.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPoints.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkMultiGroupDataInformation.h"
#include "vtkCompositeDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkActor.h"
#include "vtkMapper.h"
#include "vtkDataSet.h"
#include "vtkGeometryFilter.h"

#include "vtkSmartPointer.h"
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkSelectDataSets, "1.2");
vtkStandardNewMacro(vtkSelectDataSets);

struct vtkSelectDataSetsInternals
{
  vtkstd::vector<vtkSmartPointer<vtkDataObject> > Props;
};

//----------------------------------------------------------------------------
vtkSelectDataSets::vtkSelectDataSets()
{
  //we are a source, and don't take any inputs in the pipeline
  this->SetNumberOfInputPorts(0);
  //we just use one output port...
  //...however that output will hold multiple datasets in a CompositeDataSet
  this->SetNumberOfOutputPorts(1);

  this->Internal = new vtkSelectDataSetsInternals;
  this->Internal->Props.resize(0);
}

//----------------------------------------------------------------------------
vtkSelectDataSets::~vtkSelectDataSets()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkSelectDataSets::Initialize()
{
  //the smart pointers should delete the objects contained 
  this->Internal->Props.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectDataSets::AddDataSet(vtkObject *obj)
{
  vtkDataSet *dObj = NULL;
  vtkAlgorithm *alg = vtkAlgorithm::SafeDownCast(obj);
  if (alg)
    {
    dObj = vtkDataSet::SafeDownCast(alg->GetOutputDataObject(0));
    if (!dObj)
      {
      vtkErrorMacro("Could not find algorithm's vtkDataObject output.");
      return;
      }
    }

  int nprops = this->GetNumProps();
  this->Internal->Props.resize(nprops+1);
  this->Internal->Props[nprops] = dObj;  
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkSelectDataSets::GetNumProps()
{
  return this->Internal->Props.size();
}

//----------------------------------------------------------------------------
int vtkSelectDataSets::FillOutputPortInformation(int, vtkInformation *info)
{
  //we produce a composite (blocked not grouped) data set
  //where each block will contain a data object
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_TYPE_NAME(), 
            "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkSelectDataSets::RequestInformation(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);

  //we are not set up for streaming
  info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), 1);

  //we produce composite output
  vtkMultiGroupDataInformation *compInfo = vtkMultiGroupDataInformation::New();

  //one block for each dataset we produce
  int numBlocks = this->GetNumProps();
  compInfo->SetNumberOfGroups(numBlocks);

  //each block contains one data set
  for (int i=0; i<numBlocks; i++)
    {
    compInfo->SetNumberOfDataSets(i, 1);
    }

  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION(),compInfo);
  //cleanup
  compInfo->Delete();

  return 1;
}


//----------------------------------------------------------------------------
int vtkSelectDataSets::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{ 
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkDataObject* doOutput = 
    info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkMultiBlockDataSet* mb = 
    vtkMultiBlockDataSet::SafeDownCast(doOutput);
  if (!mb)
    {
    return 0;
    }

  //one block for each dataset we produce
  int numBlocks = this->GetNumProps();
  mb->SetNumberOfBlocks(numBlocks);
  //each block contains one data set
  for (int i=0; i<numBlocks; i++)
    {
    vtkDataObject* dObj = this->Internal->Props[i];
    mb->SetDataSet(i, 0, dObj);    
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkSelectDataSets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

